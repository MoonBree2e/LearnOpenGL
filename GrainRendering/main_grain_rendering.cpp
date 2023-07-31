#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <random>

#include <glad/glad.h> 
#include <GLFW/glfw3.h>

#include "stb_image.h"
#include "GrCamera.h"
#include "shader.h"
#include "PointCloud.h"
#include "GlBuffer.h"
#include "Light.h"
#include "ScopedFramebufferOverride.h"
#include "DebugGroup.h"
#include "GlTexture.h"

const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;
#define CLEAR_DEPTH_VALUE -1000000.0f

void processInput(GLFWwindow* window);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);

#pragma region Camera Define
std::shared_ptr<TurntableCamera> pGrCamera;
bool m_isMouseMoveStarted;
int m_isControlPressed = 0;
bool m_imguiFocus = false;

#define forAllCameras(method) \
if(pGrCamera->properties().controlInViewport) {\
    pGrCamera->method();\
}

void key_callback(GLFWwindow * window, int key, int scancode, int action, int mode) {
    
}

void mouse_button_callback(GLFWwindow * window, int button, int action, int mods) {
    if (m_imguiFocus) {
        return;
    }

    m_isMouseMoveStarted = action == GLFW_PRESS;

    switch (button) {
    case GLFW_MOUSE_BUTTON_LEFT:
        if (action == GLFW_PRESS) {
            forAllCameras(startMouseRotation);
        }
        else {
            forAllCameras(stopMouseRotation);
        }
        break;

    case GLFW_MOUSE_BUTTON_MIDDLE:
        if (action == GLFW_PRESS) {
            forAllCameras(startMouseZoom);
        }
        else {
            forAllCameras(stopMouseZoom);
        }
        break;

    case GLFW_MOUSE_BUTTON_RIGHT:
        if (action == GLFW_PRESS) {
            forAllCameras(startMousePanning);
        }
        else {
            forAllCameras(stopMousePanning);
        }
        break;
    }
}

void cursor_pos_callback(GLFWwindow* window, double x, double y) {
    //double limit = static_cast<double>(m_windowWidth) - static_cast<double>(m_panelWidth);
    //m_imguiFocus = x >= limit && m_showPanel && !m_isMouseMoveStarted;

    if (m_imguiFocus) {
        return;
    }

    if (pGrCamera->properties().controlInViewport) {
        pGrCamera->updateMousePosition(static_cast<float>(x), static_cast<float>(y));
    }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    if (m_imguiFocus) {
        return;
    }

    if (pGrCamera->properties().controlInViewport) {
        pGrCamera->zoom(-2 * static_cast<float>(yoffset));
    }
}

#pragma endregion

float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

float deltaTime = 0.0f;
float lastFrame = 0.0f;
float cameraSpeedScale = 0.5f;

const std::string ROOT_DIR = "D:/Repo/GrainViewer/share/scenes/";

GLsizei frameCount;
GLuint pointCount;
std::unique_ptr<GlBuffer> pointBuffer; 
GLuint pointVAO;

GLuint worldVAO;
GLuint worldVBO;

std::vector<std::shared_ptr<Light>> m_lights;

enum class RenderModel {
    Instance = 0,
    Impostor,
    Point,
    None,
};

struct Counter {
    GLuint count = 0;
    GLuint offset = 0;
};
enum class StepShaderVariant {
    STEP_PRECOMPUTE,
    STEP_RESET,
    STEP_COUNT,
    STEP_OFFSET,
    STEP_WRITE,
};

std::vector<Counter> m_counters;
std::unique_ptr<GlBuffer> m_countersSsbo;
std::shared_ptr<GlBuffer> m_elementBuffer;
std::unique_ptr<GlBuffer> m_renderTypeCache;
int m_local_size_x = 128;
int m_xWorkGroups;
GLuint m_elementCount;
GLuint uRenderModelCount = 4;
unsigned int colormap;

glm::mat4 modelMatrix() {
    glm::mat4 model = glm::mat4(
        0.001, 0, 0, 0,
        0, 0, 0.001, 0,
        0, 0.001, 0, 0,
        0, 0, 0, 1
    );
    return model;
}

struct PropertiesFarGrain {
    float radius = 0.007f;
    float epsilonFactor = 10.0f; // multiplied by radius
    bool useShellCulling = true;
    bool shellDepthFalloff = true;
    bool useEarlyDepthTest = true;
    bool noDiscard = false; // in eps (some artefacts on edges or with low shell depth, but faster)
    bool pseudoLean = false;

    bool useBbox = false; // if true, remove all points out of the supplied bounding box
    glm::vec3 bboxMin = glm::vec3(0);
    glm::vec3 bboxMax = glm::vec3(0);

    // For debug
    bool checkboardSprites = false;
    bool showSampleCount = false;
};

void initWorldVAO() {
    static bool worldVAOInited = false;
    if (worldVAOInited) return;

    GLfloat attributes[] = {
    -1.0f,  1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,
    1.0f, -1.0f, -1.0f,
    1.0f, -1.0f, -1.0f,
    1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,

    1.0f, -1.0f, -1.0f,
    1.0f, -1.0f,  1.0f,
    1.0f,  1.0f,  1.0f,
    1.0f,  1.0f,  1.0f,
    1.0f,  1.0f, -1.0f,
    1.0f, -1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
    1.0f,  1.0f,  1.0f,
    1.0f,  1.0f,  1.0f,
    1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,

    -1.0f,  1.0f, -1.0f,
    1.0f,  1.0f, -1.0f,
    1.0f,  1.0f,  1.0f,
    1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
    1.0f, -1.0f, -1.0f,
    1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
    1.0f, -1.0f,  1.0f
    };
    glCreateVertexArrays(1, &worldVAO);
    glBindVertexArray(worldVAO);
    glCreateBuffers(1, &worldVBO);
    glBindBuffer(GL_ARRAY_BUFFER, worldVBO);
    glNamedBufferData(worldVBO, sizeof(attributes), attributes, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexArrayAttrib(worldVAO, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

}

// imposterAtlas
std::unique_ptr<GlTexture> normalAlphaTexture;
std::unique_ptr<GlTexture> baseColorTexture;
std::unique_ptr<GlTexture> metallicRoughnessTexture;
glm::vec3 baseColor;
float metallic = 0.0;
float roughness = 0.5;
GLuint viewCount;

void loadAtlas() {
    baseColorTexture = ResourceManager::loadTextureStack("D:/Repo/GrainViewer/share/scenes/Impostors/scene/baseColor");
    normalAlphaTexture = ResourceManager::loadTextureStack("D:/Repo/GrainViewer/share/scenes/Impostors/scene/normal");
    metallicRoughnessTexture = ResourceManager::loadTextureStack("D:/Repo/GrainViewer/share/scenes/Impostors/scene/metallicRoughness");
    GLuint n = normalAlphaTexture->depth();
    viewCount = static_cast<GLuint>(sqrt(n / 2));
}

int main()
{
#pragma region BEGIN
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    const char* glsl_version = "#version 450";

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "GrainRendering", NULL, NULL);
    if (window == NULL) {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, cursor_pos_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, scroll_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        return -1;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);
    bool show_demo_window = false;
    bool show_another_window = true;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    pGrCamera = std::make_shared<TurntableCamera>();
    pGrCamera->setProjectionType(GrCamera::PerspectiveProjection);
    pGrCamera->setFov(24);
    pGrCamera->setNearDistance(0.1);
    pGrCamera->setFarDistance(100);
    pGrCamera->m_center = glm::vec3(-0.0618722, -0.0389748, 0.0787444);
    pGrCamera->m_quat = glm::quat(-0.543829, 0.437443, 0.448886, 0.558042);
    pGrCamera->m_zoom = 0.590913;
    pGrCamera->m_sensitivity = 0.003;
    pGrCamera->m_zoomSensitivity = 0.01;
    pGrCamera->updateViewMatrix();


#pragma endregion
    // --------- load textures --------
    {
        int width1, height1, nrChannels1;
        stbi_set_flip_vertically_on_load(true); // tell stb_image.h to flip loaded texture's on the y-axis.
        unsigned char* data1 = stbi_load((ROOT_DIR + "Textures/colormap2.png").c_str(), &width1, &height1, &nrChannels1, 4);

        glCall(glGenTextures(1, &colormap));
        glCall(glBindTexture(GL_TEXTURE_2D, colormap));
        glCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT));
        glCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT));
        glCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_REPEAT));
        glCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
        glCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR));

        glCall(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width1, height1, 0, GL_RGBA, GL_UNSIGNED_BYTE, data1));
        glCall(glGenerateMipmap(GL_TEXTURE_2D));
        glCall(glBindTexture(GL_TEXTURE_2D, 0));
        stbi_image_free(data1);
    }
    
    loadAtlas();



    // --------- load points --------
    {
        PointCloud pointCloud;
        pointCloud.load(ROOT_DIR + "PointClouds/20millions.bin");
        pointCount = static_cast<GLsizei>(pointCloud.data().size());
        frameCount = static_cast<GLsizei>(pointCloud.frameCount());

        pointBuffer = std::make_unique<GlBuffer>(GL_ARRAY_BUFFER);
        pointBuffer->addBlock<glm::vec4>(pointCount);
        pointBuffer->addBlockAttribute(0, 4);  // position
        pointBuffer->alloc();
        pointBuffer->fillBlock<glm::vec4>(0, [&pointCloud](glm::vec4* data, size_t _) {
            glm::vec4* v = data;
            for (auto p : pointCloud.data()) {
                v->x = p.x; v->y = p.y; v->z = p.z; v++;
            }
        });

        glCreateVertexArrays(1, &pointVAO);
        glBindVertexArray(pointVAO);
        pointBuffer->bind();
        pointBuffer->enableAttributes(pointVAO);
        glBindVertexArray(0);
        pointBuffer->finalize(); // This buffer will never be mapped on CPU
        glObjectLabel(GL_BUFFER, pointBuffer->name(), -1, "point Buffer");
    }

    // --------- build shadow map --------
    {
        auto pos = glm::vec3(5, 10, 7.5);
        auto col = glm::vec3(1.15, 1.12, 1.1);
        size_t shadowMapSize = 2048;
        bool hasShadowMap = true;
        bool isShadowMapRich = false;
        float shadowMapFov = 10.0;
        float shadowMapNear = 12;
        float shadowMapFar = 14;
        auto light = std::make_shared<Light>(pos, col, shadowMapSize, isShadowMapRich, hasShadowMap);
        light->shadowMap().setProjection(shadowMapFov, shadowMapNear, shadowMapFar);
        m_lights.push_back(light);

        pos = glm::vec3(5, -3, 4);
        col = glm::vec3(0.2, 0.24, 0.4);
        hasShadowMap = false;
        light = std::make_shared<Light>(pos, col, shadowMapSize, isShadowMapRich, hasShadowMap);
        m_lights.push_back(light);
    }
    
    Shader particleShader("particle.vert", "particle.frag");
    Shader m_occlusionCullingShader("occlusion-culling.vert", "occlusion-culling.frag");
    Shader splitter[4] = {"globalatomic-splitter-step-reset.comp",
        "globalatomic-splitter-step-count.comp",
        "globalatomic-splitter-step-offset.comp",
        "globalatomic-splitter-step-write.comp"};
    Shader imposter_grain("impostor-grain.vert", "impostor-grain.frag", "impostor-grain.geo");
    Shader far_grain("far-grain.vert", "far-grain.frag", "far-grain.geo");
    Shader worldShader("basic-world.vert", "basic-world.frag");
    Shader imposter_grain_rendering("impostor-grain-rendering.vert", "impostor-grain-rendering.frag", "impostor-grain-rendering.geo");
    
    glObjectLabel(GL_PROGRAM, m_occlusionCullingShader.ID, -1, "occlusionCulling");
    glObjectLabel(GL_PROGRAM, splitter[0].ID, -1, "splitter compute reset");
    glObjectLabel(GL_PROGRAM, splitter[1].ID, -1, "splitter compute count");
    glObjectLabel(GL_PROGRAM, splitter[2].ID, -1, "splitter compute offset");
    glObjectLabel(GL_PROGRAM, splitter[3].ID, -1, "splitter compute write");

    glObjectLabel(GL_PROGRAM, far_grain.ID, -1, "far-grain shadowmap");
    glObjectLabel(GL_PROGRAM, worldShader.ID, -1, "basic-world program");
    glObjectLabel(GL_PROGRAM, imposter_grain_rendering.ID, -1, "impostor-grain-rendering program");

    
    //Shader shaderParticles("particle.vert", "particle.frag");
    //Shader depthShader("depth.vert", "depth.frag");
    //Shader thicknessShader("thickness.vert", "thickness.frag");
    //Shader normalShader("quad.vert", "normal.frag");
    //Shader depthGaussianBlurShader("quad.vert", "depthGaussianBlur.frag");

    bool drawParticles = false;
    bool drawDepth = true;

    int frameIndex = 0;

    // init splitter
    {
        m_elementBuffer = std::make_unique<GlBuffer>(GL_ELEMENT_ARRAY_BUFFER);
        m_elementBuffer->addBlock<GLuint>(pointCount);
        m_elementBuffer->alloc();
        m_elementBuffer->finalize();
        glObjectLabel(GL_BUFFER, m_elementBuffer->name(), -1, "elementBuffer");

        m_counters.resize(4);
        m_countersSsbo = std::make_unique<GlBuffer>(GL_ELEMENT_ARRAY_BUFFER);
        m_countersSsbo->importBlock(m_counters);
        glObjectLabel(GL_BUFFER, m_countersSsbo->name(), -1, "countersSsbo");


        m_xWorkGroups = (pointCount + (m_local_size_x - 1)) / m_local_size_x;
    }

    while (!glfwWindowShouldClose(window))
    {
        processInput(window);

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        //{
        //    particleShader.use();
        //    //particleShader.setMat4("viewMatrix", camera.getViewMatrix());
        //    //particleShader.setMat4("projectionMatrix", camera.getProjectionMatrix(SCR_WIDTH, SCR_HEIGHT));
        //    glBindVertexArray(pointVAO);
        //    glPointSize(20);
        //    glDrawArrays(GL_POINTS, 0, pointCount);
        //}
        {
            // --------- point cloud splitter preRender --------
            for (const auto& light : m_lights) {
                if (!light->hasShadowMap()) continue;
                light->shadowMap().bind();
                const glm::vec2& sres = light->shadowMap().camera().resolution();
                glViewport(0, 0, static_cast<GLsizei>(sres.x), static_cast<GLsizei>(sres.y));
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                glEnable(GL_DEPTH_TEST);
                const GrCamera& lightCamera = light->shadowMap().camera();
                std::shared_ptr<Framebuffer> occlusionCullingFbo;
                occlusionCullingFbo = lightCamera.getExtraFramebuffer("occlusionCullingFbo", GrCamera::ExtraFramebufferOption::Rgba32fDepth);
                glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "Occlusion culling map");

                glClearColor(0, 0, 0, 1);
                glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
                glEnable(GL_PROGRAM_POINT_SIZE);
                // Occlusion culling map
                {
                    // --------- z-prepass(optional) --------
                    {
                        glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "z-prepass (optional)");
                        ScopedFramebufferOverride scoppedFramebufferOverride;
                        occlusionCullingFbo->bind();
                        occlusionCullingFbo->deactivateColorAttachments();
                        Shader& shader = m_occlusionCullingShader;
                        // set uniform
                        {
                            glm::mat4 viewModelMatrix = lightCamera.viewMatrix() * modelMatrix();
                            shader.bindUniformBlock("Camera", lightCamera.ubo());
                            shader.setUniform("modelMatrix", modelMatrix());
                            shader.setUniform("viewModelMatrix", viewModelMatrix);
                            shader.setUniform("uGrainRadius", 0.0005f);
                            shader.setUniform("uGrainInnerRadiusRatio", 0.95f);
                            shader.setUniform("uOuterOverInnerRadius", 1.f / 0.95f);

                            shader.setUniform("uPointCount", pointCount);
                            shader.setUniform("uFrameCount", 1);
                            shader.setUniform("uTime", 0);
                        }
                        shader.use();
                        pointBuffer->bind();
                        pointBuffer->bindSsbo(1);
                        glDrawArrays(GL_POINTS, 0, pointCount);
                        pointBuffer->unbind();
                        glTextureBarrier();
                        occlusionCullingFbo->activateColorAttachments();
                        glBindFramebuffer(GL_FRAMEBUFFER, 0);
                        glPopDebugGroup();
                    }
                    // --------- core pass --------
                    {
                        glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "core pass");
                        ScopedFramebufferOverride scoppedFramebufferOverride;
                        occlusionCullingFbo->bind();

                        glEnable(GL_PROGRAM_POINT_SIZE);
                        //if z pre pass
                        glDepthMask(GL_FALSE);
                        glDepthFunc(GL_LEQUAL);

                        occlusionCullingFbo = lightCamera.getExtraFramebuffer("occlusionCullingFbo", GrCamera::ExtraFramebufferOption::Rgba32fDepth);
                        occlusionCullingFbo->bind();
                        glClearColor(0, 0, 0, 1);
                        glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
                        Shader& shader = m_occlusionCullingShader;

                        // set uniform
                        {
                            glm::mat4 viewModelMatrix = lightCamera.viewMatrix() * modelMatrix();
                            shader.bindUniformBlock("Camera", lightCamera.ubo());
                            shader.setUniform("modelMatrix", modelMatrix());
                            shader.setUniform("viewModelMatrix", viewModelMatrix);
                            shader.setUniform("uGrainRadius", 0.0005f);
                            shader.setUniform("uGrainInnerRadiusRatio", 0.95f);
                            shader.setUniform("uOuterOverInnerRadius", 1.f / 0.95f);

                            shader.setUniform("uPointCount", pointCount);
                            shader.setUniform("uFrameCount", 1);
                            shader.setUniform("uTime", 0);
                        }

                        shader.use();
                        pointBuffer->bind();
                        pointBuffer->bindSsbo(1);
                        glDrawArrays(GL_POINTS, 0, pointCount);
                        glBindVertexArray(0);

                        glTextureBarrier();
                        glDepthMask(GL_TRUE);
                        glDepthFunc(GL_LESS);

                        glPopDebugGroup();
                    }
                }

                // --------- splitting --------
                {
                    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "Splitting");
                    m_elementCount = pointCount;
                    if (!m_renderTypeCache) {
                        m_renderTypeCache = std::make_unique<GlBuffer>(GL_ELEMENT_ARRAY_BUFFER);
                        m_renderTypeCache->addBlock<GLuint>(pointCount);
                        m_renderTypeCache->alloc();
                        m_renderTypeCache->finalize();
                        glObjectLabel(GL_BUFFER, m_renderTypeCache->name(), -1, "renderTypeCache");
                    }
                    m_countersSsbo->bindSsbo(0);
                    m_renderTypeCache->bindSsbo(1);
                    m_elementBuffer->bindSsbo(2);
                    pointBuffer->bind();
                    pointBuffer->bindSsbo(3);

                    StepShaderVariant firstStep = StepShaderVariant::STEP_RESET;
                    constexpr StepShaderVariant lastStep = StepShaderVariant::STEP_WRITE;
                    constexpr int STEP_RESET = static_cast<int>(StepShaderVariant::STEP_RESET);
                    constexpr int STEP_OFFSET = static_cast<int>(StepShaderVariant::STEP_OFFSET);

                    for (int i = static_cast<int>(firstStep); i <= static_cast<int>(lastStep); ++i) {
                        Shader& shader = splitter[i - 1];
                        shader.use();

                        // set uniform
                        {
                            glm::mat4 viewModelMatrix = lightCamera.viewMatrix() * modelMatrix();
                            shader.bindUniformBlock("Camera", lightCamera.ubo());
                            shader.setUniform("modelMatrix", modelMatrix());
                            shader.setUniform("viewModelMatrix", viewModelMatrix);
                            shader.setUniform("uGrainRadius", 0.0005f);
                            shader.setUniform("uGrainInnerRadiusRatio", 0.95f);
                            shader.setUniform("uOuterOverInnerRadius", 1.f / 0.95f);

                            shader.setUniform("uFrameCount", 1);
                            shader.setUniform("uTime", 0);
                            shader.setUniform("uPointCount", m_elementCount);
                            shader.setUniform("uRenderModelCount", uRenderModelCount);
                        }

                        glBindTextureUnit(0, occlusionCullingFbo->colorTexture(0));
                        shader.setUniform("uOcclusionMap", 0);

                        glDispatchCompute(i == STEP_RESET || i == STEP_OFFSET ? 1 : static_cast<GLuint>(m_xWorkGroups), 1, 1);
                        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
                    }

                    // Get counters back
                    m_countersSsbo->exportBlock(0, m_counters);
                    glPopDebugGroup();
                }
                glPopDebugGroup();

                // --------- main shadow map rendering --------
                glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "main shadow map rendering");
                glEnable(GL_PROGRAM_POINT_SIZE);
                {
                    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "far-grain");

                    glEnable(GL_PROGRAM_POINT_SIZE);
                    glDepthMask(GL_TRUE);
                    glDisable(GL_BLEND);

                    Shader& shader = far_grain;
                    glm::mat4 viewModelMatrix = lightCamera.viewMatrix() * modelMatrix();
                    PropertiesFarGrain properties;
                    properties.epsilonFactor = 0.445;
                    float grainRadius = 0.0005;
                    {
                        shader.bindUniformBlock("Camera", lightCamera.ubo());
                        shader.setUniform("modelMatrix", modelMatrix());
                        shader.setUniform("viewModelMatrix", viewModelMatrix);
                        shader.setUniform("uFrameCount", 1);
                        shader.setUniform("uTime", 0);
                        shader.setUniform("uGrainInnerRadiusRatio", 0.95f);
                        shader.setUniform("uGrainRadius", 0.0005f);
                        shader.setUniform("uEpsilon", properties.epsilonFactor * grainRadius);
                        shader.setUniform("uBboxMin", properties.bboxMin);
                        shader.setUniform("uBboxMax", properties.bboxMax);
                        shader.setUniform("uuseBbox", properties.useBbox);
                        shader.setUniform("uShellDepthFalloff", (GLuint)1);
                        shader.setUniform("uWeightMode", 0);

                        glBindTextureUnit(0, colormap);
                        shader.setUniform("uColormapTexture", 0);
                    }
                    shader.use();
                    glBindVertexArray(pointVAO);
                    m_elementBuffer->bindSsbo(1);
                    pointBuffer->bindSsbo(0);
                    int offset = m_counters[static_cast<int>(RenderModel::Point)].offset;
                    int count = m_counters[static_cast<int>(RenderModel::Point)].count;
                    glDrawArrays(GL_POINTS, offset, count);
                    glBindVertexArray(0);

                    glPopDebugGroup();
                }
                glPopDebugGroup();
            }
        }

        {
            glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "renderCamera");
            GrCamera& camera = *pGrCamera;
            const glm::vec2& res = camera.resolution();
            const glm::vec4& rect = camera.properties().viewRect;
            GLint vX0 = static_cast<GLint>(rect.x * SCR_WIDTH);
            GLint vY0 = static_cast<GLint>(rect.y * SCR_HEIGHT);
            GLsizei vWidth = static_cast<GLsizei>(res.x);
            GLsizei vHeight = static_cast<GLsizei>(res.y);
	        GLint vX = 0, vY = 0;

            auto DeferredFBO = camera.getExtraFramebuffer("Camera Deferred FBO", GrCamera::ExtraFramebufferOption::GBufferDepth);
            DeferredFBO->bind();
            glViewport(vX, vY, vWidth, vHeight);
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // Pre-rendering -- occlusion
            {
                std::shared_ptr<Framebuffer> occlusionCullingFbo;

                glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "occlusion");
                GrCamera& prerenderCamera = camera;
                {
                    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "PointCloudSplitter PreRender");
                    ScopedFramebufferOverride scoppedFramebufferOverride;
                    glEnable(GL_PROGRAM_POINT_SIZE);

                    occlusionCullingFbo = camera.getExtraFramebuffer("Camera occlusionCullingFbo", GrCamera::ExtraFramebufferOption::Rgba32fDepth);
                    occlusionCullingFbo->bind();
                    glClearColor(0, 0, 0, 1);
                    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

                    // --------- z-prepass (optional) ---------
                    {
                        glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "z-prepass (optional)");
                        occlusionCullingFbo->deactivateColorAttachments();
                        Shader& shader = m_occlusionCullingShader;
                        // set uniform
                        {
                            glm::mat4 viewModelMatrix = prerenderCamera.viewMatrix() * modelMatrix();
                            shader.bindUniformBlock("Camera", prerenderCamera.ubo());
                            shader.setUniform("modelMatrix", modelMatrix());
                            shader.setUniform("viewModelMatrix", viewModelMatrix);
                            shader.setUniform("uGrainRadius", 0.0005f);
                            shader.setUniform("uGrainInnerRadiusRatio", 0.95f);
                            shader.setUniform("uOuterOverInnerRadius", 1.f / 0.95f);

                            shader.setUniform("uPointCount", pointCount);
                            shader.setUniform("uFrameCount", 1);
                            shader.setUniform("uTime", 0);
                        }
                        shader.use();
                        pointBuffer->bind();
                        pointBuffer->bindSsbo(1);
                        glDrawArrays(GL_POINTS, 0, pointCount);
                        glBindVertexArray(0);

                        glTextureBarrier();
                        occlusionCullingFbo->activateColorAttachments();

                        glPopDebugGroup();
                    }

                    // --------- core pass ---------
                    {
                        glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "core pass");
                        ScopedFramebufferOverride scoppedFramebufferOverride;
                        occlusionCullingFbo->bind();

                        glEnable(GL_PROGRAM_POINT_SIZE);
                        //if z pre pass
                        glDepthMask(GL_FALSE);
                        glDepthFunc(GL_LEQUAL);

                        occlusionCullingFbo = prerenderCamera.getExtraFramebuffer("Camera occlusionCullingFbo", GrCamera::ExtraFramebufferOption::Rgba32fDepth);
                        occlusionCullingFbo->bind();
                        glClearColor(0, 0, 0, 1);
                        glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
                        Shader& shader = m_occlusionCullingShader;

                        // set uniform
                        {
                            glm::mat4 viewModelMatrix = prerenderCamera.viewMatrix() * modelMatrix();
                            shader.bindUniformBlock("Camera", prerenderCamera.ubo());
                            shader.setUniform("modelMatrix", modelMatrix());
                            shader.setUniform("viewModelMatrix", viewModelMatrix);
                            shader.setUniform("uGrainRadius", 0.0005f);
                            shader.setUniform("uGrainInnerRadiusRatio", 0.95f);
                            shader.setUniform("uOuterOverInnerRadius", 1.f / 0.95f);

                            shader.setUniform("uPointCount", pointCount);
                            shader.setUniform("uFrameCount", 1);
                            shader.setUniform("uTime", 0);
                        }

                        shader.use();
                        pointBuffer->bind();
                        pointBuffer->bindSsbo(1);
                        glDrawArrays(GL_POINTS, 0, pointCount);
                        glBindVertexArray(0);

                        glTextureBarrier();
                        glDepthMask(GL_TRUE);
                        glDepthFunc(GL_LESS);

                        glPopDebugGroup();
                    }
                    // --------- Splitting ---------
                    {
                        DebugGroupOverrride debugGroup("Splitting");
                        m_elementCount = pointCount;
                        if (!m_renderTypeCache) {
                            m_renderTypeCache = std::make_unique<GlBuffer>(GL_ELEMENT_ARRAY_BUFFER);
                            m_renderTypeCache->addBlock<GLuint>(pointCount);
                            m_renderTypeCache->alloc();
                            m_renderTypeCache->finalize();
                            glObjectLabel(GL_BUFFER, m_renderTypeCache->name(), -1, "renderTypeCache");
                        }
                        m_countersSsbo->bindSsbo(0);
                        m_renderTypeCache->bindSsbo(1);
                        m_elementBuffer->bindSsbo(2);
                        pointBuffer->bind();
                        pointBuffer->bindSsbo(3);

                        StepShaderVariant firstStep = StepShaderVariant::STEP_RESET;
                        constexpr StepShaderVariant lastStep = StepShaderVariant::STEP_WRITE;
                        constexpr int STEP_RESET = static_cast<int>(StepShaderVariant::STEP_RESET);
                        constexpr int STEP_OFFSET = static_cast<int>(StepShaderVariant::STEP_OFFSET);

                        for (int i = static_cast<int>(firstStep); i <= static_cast<int>(lastStep); ++i) {
                            Shader& shader = splitter[i - 1];
                            shader.use();

                            // set uniform
                            {
                                glm::mat4 viewModelMatrix = prerenderCamera.viewMatrix() * modelMatrix();
                                shader.bindUniformBlock("Camera", prerenderCamera.ubo());
                                shader.setUniform("modelMatrix", modelMatrix());
                                shader.setUniform("viewModelMatrix", viewModelMatrix);
                                shader.setUniform("uGrainRadius", 0.0005f);
                                shader.setUniform("uGrainInnerRadiusRatio", 0.95f);
                                shader.setUniform("uOuterOverInnerRadius", 1.f / 0.95f);

                                shader.setUniform("uFrameCount", 1);
                                shader.setUniform("uTime", 0);
                                shader.setUniform("uPointCount", m_elementCount);
                                shader.setUniform("uRenderModelCount", uRenderModelCount);
                            }

                            glBindTextureUnit(0, occlusionCullingFbo->colorTexture(0));
                            shader.setUniform("uOcclusionMap", 0);

                            glDispatchCompute(i == STEP_RESET || i == STEP_OFFSET ? 1 : static_cast<GLuint>(m_xWorkGroups), 1, 1);
                            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
                        }

                        // Get counters back
                        m_countersSsbo->exportBlock(0, m_counters);
                    }


                    glPopDebugGroup();

                }
                glPopDebugGroup();
            }

            // main rendering
            {
                DebugGroupOverrride debugGroup("main renderring");
                // --------- world --------
                {
                    DebugGroupOverrride debugGroup("basic-world");

                    glDepthMask(GL_FALSE);
                    glDepthFunc(GL_LEQUAL);
                    worldShader.use();
                    worldShader.bindUniformBlock("Camera", camera.ubo());

                    initWorldVAO();
                    glBindVertexArray(worldVAO);
                    glDrawArrays(GL_TRIANGLES, 0, 6 * 6);
                    glBindVertexArray(0);
                    glDepthMask(GL_TRUE);
                    glDepthFunc(GL_LESS);
                }
                // --------- imposter-grain rendering --------
                {
                    DebugGroupOverrride debugGroup("imposter-grain");
                    ScopedFramebufferOverride scoppedFramebufferOverride; // to automatically restore fbo binding at the end of scope

                    Shader& shader = imposter_grain_rendering;
                    shader.use();
                    // setUniforms
                    {
                        glm::mat4 viewModelMatrix = camera.viewMatrix() * modelMatrix();
                        shader.bindUniformBlock("Camera", camera.ubo());
                        shader.setUniform("modelMatrix", modelMatrix());
                        shader.setUniform("viewModelMatrix", viewModelMatrix);
                        shader.setUniform("uGrainRadius", 0.0005f);
                        

                        shader.setUniform("uFrameCount", 1);
                        shader.setUniform("uTime", 0);

                        GLint o = 0;
                        glBindTextureUnit(o, colormap);
                        shader.setUniform("uColormapTexture", o++);

                        std::string prefix = "uImpostor[0].";
                        shader.setUniform(prefix + "viewCount", viewCount);
                        shader.setUniform(prefix + "baseColor", baseColor);
                        shader.setUniform(prefix + "metallic", metallic);
                        shader.setUniform(prefix + "roughness", roughness);

                        normalAlphaTexture->bind(o);
                        shader.setUniform(prefix + "normalAlphaTexture", o++);

                        if (baseColorTexture) {
                            baseColorTexture->bind(o);
                            shader.setUniform(prefix + "baseColorTexture", o++);
                        }
                        shader.setUniform(prefix + "hasBaseColorMap", static_cast<bool>(baseColorTexture));

                        if (metallicRoughnessTexture) {
                            metallicRoughnessTexture->bind(o);
                            shader.setUniform(prefix + "metallicRoughnessTexture", o++);
                        }
                        shader.setUniform(prefix + "hasMetallicRoughnessMap", static_cast<bool>(metallicRoughnessTexture));

                        auto occlusionCullingFbo = camera.getExtraFramebuffer("occlusionCullingFbo", GrCamera::ExtraFramebufferOption::Rgba32fDepth);
                        glBindTextureUnit(static_cast<GLuint>(o), occlusionCullingFbo->colorTexture(0));
                        shader.setUniform("uOcclusionMap", o++);
                        shader.setUniform("uUseOcclusionMap", true);
                    }
                    // draw
                    {
                        shader.setUniform("uPrerenderSurfaceStep", 0);

                        shader.use();
                        glBindVertexArray(pointVAO);
                        pointBuffer->bindSsbo(0);
                        m_elementBuffer->bindSsbo(1);
                        shader.setUniform("uUsePointElements", true);
                        int offset = m_counters[1].offset; // 1 for imposter
                        int count = m_counters[1].count;
                        glDrawArrays(GL_POINTS, offset, count);
                        glBindVertexArray(0);
                    }
                    // draw PrerenderSurfaceStep
                    {
                        shader.setUniform("uPrerenderSurfaceStep", 1);

                        shader.use();
                        glBindVertexArray(pointVAO);
                        pointBuffer->bindSsbo(0);
                        m_elementBuffer->bindSsbo(1);
                        shader.setUniform("uUsePointElements", true);
                        int offset = m_counters[1].offset; // 1 for imposter
                        int count = m_counters[1].count;
                        glDrawArrays(GL_POINTS, offset, count);
                        glBindVertexArray(0);
                    }
                }
            }



            glPopDebugGroup();
        }


        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (show_another_window)
        {
            ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
            ImGui::Text("Hello from another window!");
            //ImGui::DragFloat("pointScale", &pointScale, 10);
            //ImGui::DragFloat("pointSize", &pointSize, 0.0005f);

            auto desiredWidth = ImGui::GetContentRegionAvail().x;
            auto scale = desiredWidth / SCR_WIDTH;
            auto imageDrawSize = ImVec2(desiredWidth, SCR_HEIGHT * scale);
            auto drawImage = [&](const char* name, GLuint tex) {
                ImGui::Text(name);
                ImGui::Image(reinterpret_cast<ImTextureID>(tex), imageDrawSize, { 0, 1 }, { 1, 0 });
            };
            //ImGui::Text("aaaa");
//            if (ImGui::CollapsingHeader("depth")) drawImage("depth", depthTexture);
//            if (ImGui::CollapsingHeader("thickness")) drawImage("thickness", thicknessTexture);
//#if 1 
//            if (ImGui::CollapsingHeader("depthGaussian")) drawImage("depthGaussian", depthGaussianBlurTarget);
//#endif
            ImGui::End();
        }

        // Rendering
        ImGui::Render();

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
        frameIndex++;
    }

    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    float currentFrame = static_cast<float>(glfwGetTime());
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;
    float t = cameraSpeedScale * deltaTime;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}
