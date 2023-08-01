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
#include "PostEffect.h"

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

#define MAKE_STR(contents) static_cast<std::ostringstream&&>(std::ostringstream() << contents).str()

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
unsigned int defferedShadingColormap;

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

GLuint defferedVAO;



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
        stbi_set_flip_vertically_on_load(false); // tell stb_image.h to flip loaded texture's on the y-axis.
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

        data1 = stbi_load((ROOT_DIR + "Textures/turbo.png").c_str(), &width1, &height1, &nrChannels1, 4);
        glCall(glGenTextures(1, &defferedShadingColormap));
        glCall(glBindTexture(GL_TEXTURE_2D, defferedShadingColormap));
        glCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
        glCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
        glCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE));
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

        m_elementCount = pointCount;
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
    Shader far_rain_render_depth("far-grain-render-depth.vert", "far-grain-render-depth.frag", "far-grain-render-depth.geo");
    Shader far_rain_render_point("far-grain-render-point.vert", "far-grain-render-point.frag", "far-grain-render-point.geo");
    Shader far_rain_render_blit("far-grain-render-blit.vert", "far-grain-render-blit.frag", "far-grain-render-blit.geo");
    Shader deffered_shader("deferred-shader.vert", "deferred-shader.frag", "deferred-shader.geo");
    glCreateVertexArrays(1, &defferedVAO);

    glObjectLabel(GL_PROGRAM, m_occlusionCullingShader.ID, -1, "occlusionCulling");
    glObjectLabel(GL_PROGRAM, splitter[0].ID, -1, "splitter compute reset");
    glObjectLabel(GL_PROGRAM, splitter[1].ID, -1, "splitter compute count");
    glObjectLabel(GL_PROGRAM, splitter[2].ID, -1, "splitter compute offset");
    glObjectLabel(GL_PROGRAM, splitter[3].ID, -1, "splitter compute write");

    glObjectLabel(GL_PROGRAM, far_grain.ID, -1, "far-grain shadowmap");
    glObjectLabel(GL_PROGRAM, worldShader.ID, -1, "basic-world program");
    glObjectLabel(GL_PROGRAM, imposter_grain_rendering.ID, -1, "impostor-grain-rendering program");
    glObjectLabel(GL_PROGRAM, far_rain_render_depth.ID, -1, "far-grain render depth");
    glObjectLabel(GL_PROGRAM, far_rain_render_point.ID, -1, "far-grain render point");
    glObjectLabel(GL_PROGRAM, far_rain_render_blit.ID, -1, "far-grain render blit");
    glObjectLabel(GL_PROGRAM, deffered_shader.ID, -1, "deffered_shader");

    
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

        // --------- render shadow maps --------
        {
            DebugGroupOverrride debugGroup("render shadow maps");

            // --------- point cloud splitter preRender --------
            for (const auto& light : m_lights) {
                if (!light->hasShadowMap()) continue;
                light->shadowMap().bind();

                const glm::vec2& sres = light->shadowMap().camera().resolution();
                glViewport(0, 0, static_cast<GLsizei>(sres.x), static_cast<GLsizei>(sres.y));
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                glEnable(GL_DEPTH_TEST);

                const GrCamera& lightCamera = light->shadowMap().camera();

                // --------- each light pre rendering ---------
                {
                    std::shared_ptr<Framebuffer> occlusionCullingFbo;

                    // 1. Occlusion culling map (kind of shadow map)
                    {
                        DebugGroupOverrride debugGroup("1. Occlusion culling map");
                        ScopedFramebufferOverride scoppedFramebufferOverride;
                        glEnable(GL_PROGRAM_POINT_SIZE);

                        occlusionCullingFbo = lightCamera.getExtraFramebuffer("occlusionCullingFbo", GrCamera::ExtraFramebufferOption::Rgba32fDepth);
                        occlusionCullingFbo->bind();

                        glClearColor(0, 0, 0, 1);
                        glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

                        // 1.1 z-prepass (optional)
                        {
                            DebugGroupOverrride debugGroup("1.1 z-prepass (optional)");
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

                                shader.setUniform("uPointCount", m_elementCount);
                                shader.setUniform("uFrameCount", 1);
                                shader.setUniform("uTime", 0);

                                shader.use();

                                glBindVertexArray(pointVAO);
                                pointBuffer->bind();
                                pointBuffer->bindSsbo(1);
                                glDrawArrays(GL_POINTS, 0, m_elementCount);
                                glBindVertexArray(0);

                                glTextureBarrier();
                                occlusionCullingFbo->activateColorAttachments();
                            }
                        }
                        // 1.2. core pass
                        {
                            DebugGroupOverrride debugGroup("1.2 core pass");
                            ScopedFramebufferOverride scoppedFramebufferOverride;

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

                                shader.setUniform("uPointCount", m_elementCount);
                                shader.setUniform("uFrameCount", 1);
                                shader.setUniform("uTime", 0);
                            }

                            shader.use();
                            glBindVertexArray(pointVAO);

                            pointBuffer->bind();
                            pointBuffer->bindSsbo(1);
                            glDrawArrays(GL_POINTS, 0, m_elementCount);
                            glBindVertexArray(0);

                            glTextureBarrier();
                            glDepthMask(GL_TRUE);
                            glDepthFunc(GL_LESS);
                        }
                    }
                    // 2. Splitting
                    {
                        DebugGroupOverrride debugGroup("2. Splitting");
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
                                shader.setUniform("uImpostorLimit", 1.11f);
                                shader.setUniform("uInstanceLimit", 0.f);
                                shader.setUniform("uGrainInnerRadiusRatio", 0.95f);
                                shader.setUniform("uOuterOverInnerRadius", 1.f / 0.95f);

                                shader.setUniform("uFrameCount", 1);
                                shader.setUniform("uTime", 0);
                                shader.setUniform("uPointCount", m_elementCount);
                                shader.setUniform("uRenderModelCount", uRenderModelCount);
                            }
                            if (occlusionCullingFbo) {
                                glBindTextureUnit(0, occlusionCullingFbo->colorTexture(0));
                                shader.setUniform("uOcclusionMap", 0);
                            }


                            glDispatchCompute(i == STEP_RESET || i == STEP_OFFSET ? 1 : static_cast<GLuint>(m_xWorkGroups), 1, 1);
                            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
                        }

                        // Get counters back
                        m_countersSsbo->exportBlock(0, m_counters);
                    }
                }

                // --------- main shadow map rendering --------
                {
                    DebugGroupOverrride debugGroup("main shadow map rendering");

                    glEnable(GL_PROGRAM_POINT_SIZE);
                    // imposter
                    {

                    }
                    // far-grain
                    {
                        DebugGroupOverrride debugGroup("far-grain");

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
                    }
                }

            }
        }

        // --------- render camera --------
        {
            DebugGroupOverrride debugGroup("renderCamera");
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
                DebugGroupOverrride debugGroup("occlusion");
                GrCamera& prerenderCamera = camera;

                // PointCloudSplitter PreRender
                {
                    DebugGroupOverrride debugGroup("PointCloudSplitter PreRender");

                    std::shared_ptr<Framebuffer> occlusionCullingFbo;

                    // 1. Occlusion culling map (kind of shadow map)
                    {
                        DebugGroupOverrride debugGroup("Occlusion culling map");

                        ScopedFramebufferOverride scoppedFramebufferOverride;
                        glEnable(GL_PROGRAM_POINT_SIZE);

                        occlusionCullingFbo = camera.getExtraFramebuffer("Camera occlusionCullingFbo", GrCamera::ExtraFramebufferOption::Rgba32fDepth);
                        occlusionCullingFbo->bind();
                        glClearColor(0, 0, 0, 1);
                        glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

                        // --------- z-prepass (optional) ---------
                        {
                            DebugGroupOverrride debugGroup("z-prepass (optional)");

                            occlusionCullingFbo->deactivateColorAttachments();
                            Shader& shader = m_occlusionCullingShader;
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

                                shader.setUniform("uPointCount", m_elementCount);
                                shader.setUniform("uFrameCount", 1);
                                shader.setUniform("uTime", 0);
                            }
                            pointBuffer->bind();
                            pointBuffer->bindSsbo(1);
                            glBindVertexArray(pointVAO);
                            glDrawArrays(GL_POINTS, 0, m_elementCount);
                            glBindVertexArray(0);

                            glTextureBarrier();
                            occlusionCullingFbo->activateColorAttachments();
                        }

                        // --------- core pass ---------
                        {
                            DebugGroupOverrride debugGroup("core pass");

                            ScopedFramebufferOverride scoppedFramebufferOverride;
                            occlusionCullingFbo = camera.getExtraFramebuffer("occlusionCullingFbo", GrCamera::ExtraFramebufferOption::Rgba32fDepth);
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

                                shader.setUniform("uPointCount", m_elementCount);
                                shader.setUniform("uFrameCount", 1);
                                shader.setUniform("uTime", 0);
                            }

                            pointBuffer->bind();
                            pointBuffer->bindSsbo(1);
                            glBindVertexArray(pointVAO);
                            glDrawArrays(GL_POINTS, 0, m_elementCount);
                            glBindVertexArray(0);

                            glTextureBarrier();
                            glDepthMask(GL_TRUE);
                            glDepthFunc(GL_LESS);

                        }
                    }

                    // 2. Splitting
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
                                shader.setUniform("uImpostorLimit", 1.11f);
                                shader.setUniform("uInstanceLimit", 0.f);
                                shader.setUniform("uGrainInnerRadiusRatio", 0.95f);
                                shader.setUniform("uOuterOverInnerRadius", 1.f / 0.95f);

                                shader.setUniform("uFrameCount", 1);
                                shader.setUniform("uTime", 0);
                                shader.setUniform("uPointCount", m_elementCount);
                                shader.setUniform("uRenderModelCount", uRenderModelCount);
                            }
                            if (occlusionCullingFbo) {
                                glBindTextureUnit(0, occlusionCullingFbo->colorTexture(0));
                                shader.setUniform("uOcclusionMap", 0);
                            }

                            glDispatchCompute(i == STEP_RESET || i == STEP_OFFSET ? 1 : static_cast<GLuint>(m_xWorkGroups), 1, 1);
                            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
                        }

                        // Get counters back
                        m_countersSsbo->exportBlock(0, m_counters);
                    }
                }
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

                        shader.setUniform("uPointCount", (GLuint)m_counters[1].count);

                        shader.setUniform("uHitSphereCorrectionFactor", 0.9f);
                        shader.setUniform("uSamplingMode", 2);
                        shader.setUniform("uPrerenderSurface", 1);


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
                // --------- far-grain rendering --------
                {
                    ScopedFramebufferOverride scoppedFramebufferOverride;
                    DebugGroupOverrride debugGroup("far-grain-rendering");
                    glEnable(GL_PROGRAM_POINT_SIZE);

                    std::shared_ptr<Framebuffer> fbo = camera.getExtraFramebuffer("FarGrainRenderer::renderToGBuffer", GrCamera::ExtraFramebufferOption::LinearGBufferDepth);
                    // 0. clear depth
                    {
                        fbo->deactivateColorAttachments();
                        fbo->bind();
                        glClear(GL_DEPTH_BUFFER_BIT);
                    }

                    // 1. Render depth buffer with an offset of epsilon
                    {
                        Shader& shader = far_rain_render_depth;
                        glDepthMask(GL_TRUE);
                        glEnable(GL_DEPTH_TEST);
                        glDisable(GL_BLEND);

                        glm::mat4 viewModelMatrix = camera.viewMatrix() * modelMatrix();
                        PropertiesFarGrain properties;
                        properties.epsilonFactor = 0.445;
                        float grainRadius = 0.0005;
                        // set uniforms
                        {
                            shader.bindUniformBlock("Camera", camera.ubo());
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
                            shader.setUniform("uDebugShape", 0);


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
                    }

                    // 2. Clear color buffers
                    {
                        fbo->activateColorAttachments();
                        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
                        glClear(GL_COLOR_BUFFER_BIT);
                    }

                    // 3. Render points cumulatively
                    {
                        Shader& shader = far_rain_render_point;
                        shader.use();

                        glDepthMask(GL_FALSE);
                        glEnable(GL_DEPTH_TEST);
                        glEnable(GL_BLEND);
                        glBlendFunc(GL_ONE, GL_ONE);

                        glm::mat4 viewModelMatrix = camera.viewMatrix() * modelMatrix();
                        PropertiesFarGrain properties;
                        properties.epsilonFactor = 0.445;
                        float grainRadius = 0.0005;
                        // set uniforms
                        {
                            shader.bindUniformBlock("Camera", camera.ubo());
                            shader.setUniform("modelMatrix", modelMatrix());
                            shader.setUniform("viewModelMatrix", viewModelMatrix);
                            shader.setUniform("uFrameCount", 1);
                            shader.setUniform("uTime", 0);
                            shader.setUniform("uGrainInnerRadiusRatio", 0.95f);
                            shader.setUniform("uGrainRadius", 0.0005f);
                            shader.setUniform("uEpsilon", properties.epsilonFactor* grainRadius);
                            shader.setUniform("uBboxMin", properties.bboxMin);
                            shader.setUniform("uBboxMax", properties.bboxMax);
                            shader.setUniform("uuseBbox", properties.useBbox);
                            shader.setUniform("uShellDepthFalloff", (GLuint)1);
                            shader.setUniform("uWeightMode", 0);
                            shader.setUniform("uUseEarlyDepthTest", 1);
                            shader.setUniform("uDebugShape", 0);

                            GLint o = 0;
                            glBindTextureUnit(o, colormap);
                            shader.setUniform("uColormapTexture", o++);

                            // bind atlases
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

                            // bindDepthTexture
                            glTextureBarrier();
                            GLuint textureUnit = 7;
                            GLint depthTexture;
                            GLint drawFboId = 0;
                            glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &drawFboId);
                            glGetNamedFramebufferAttachmentParameteriv(drawFboId, GL_DEPTH_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &depthTexture);
                            glBindTextureUnit(textureUnit, static_cast<GLuint>(depthTexture));
                            shader.setUniform("uDepthTexture", static_cast<GLint>(textureUnit));
                        }
                        glBindVertexArray(pointVAO);
                        m_elementBuffer->bindSsbo(1);
                        pointBuffer->bindSsbo(0);
                        int offset = m_counters[static_cast<int>(RenderModel::Point)].offset;
                        int count = m_counters[static_cast<int>(RenderModel::Point)].count;
                        glDrawArrays(GL_POINTS, offset, count);
                        glBindVertexArray(0);
                    }

                    // 4. Blit extra fbo to gbuffer
                    {
                        Shader& shader = far_rain_render_blit;
                        scoppedFramebufferOverride.restore();
                        glDepthMask(GL_TRUE);
                        glEnable(GL_DEPTH_TEST);
                        glDisable(GL_BLEND);

                        // set uniforms
                        {
                            glm::mat4 viewModelMatrix = camera.viewMatrix() * modelMatrix();
                            PropertiesFarGrain properties;
                            properties.epsilonFactor = 0.445;
                            float grainRadius = 0.0005;

                            shader.bindUniformBlock("Camera", camera.ubo());
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
                        // Bind secondary FBO textures
                        {
                            glTextureBarrier();
                            GLint o = 0;
                            for (int i = 0; i < fbo->colorTextureCount(); ++i) {
                                glBindTextureUnit(static_cast<GLuint>(o), fbo->colorTexture(i));
                                shader.setUniform(MAKE_STR("lgbuffer" << i), o);
                                ++o;
                            }
                            glBindTextureUnit(static_cast<GLuint>(o), fbo->depthTexture());
                            shader.setUniform("uFboDepthTexture", o++);

                            glTextureBarrier();
                            GLuint textureUnit = o++;
                            GLint depthTexture;
                            GLint drawFboId = 0;
                            glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &drawFboId);
                            glGetNamedFramebufferAttachmentParameteriv(drawFboId, GL_DEPTH_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &depthTexture);
                            glBindTextureUnit(textureUnit, static_cast<GLuint>(depthTexture));
                            shader.setUniform("uDepthTexture", static_cast<GLint>(textureUnit));

                            shader.use();
                            PostEffect::DrawWithDepthTest();
                        }

                    }
                }
                // --------- deffered shading --------
                {
                    DebugGroupOverrride debugGroup("Deffered shading");
                    glBindFramebuffer(GL_FRAMEBUFFER, 0);
                    vX = vX0;  vY = vY0;
                    glm::vec2 blitOffset = glm::vec2(vX, vY);

                    auto fbo = camera.getExtraFramebuffer("Deferred FBO", GrCamera::ExtraFramebufferOption::GBufferDepth);
                    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
                    //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                    glDepthMask(GL_TRUE);
                    glDisable(GL_DEPTH_TEST);
                    glDisable(GL_BLEND);

                    Shader& shader = deffered_shader;
                    // set uniforms
                    {
                        shader.bindUniformBlock("Camera", camera.ubo());
                        shader.setUniform("uBlitOffset", blitOffset);

                        GLint o = 0;
                        for (int i = 0; i < fbo->colorTextureCount(); ++i) {
                            shader.setUniform(MAKE_STR("gbuffer" << i), o);
                            glBindTextureUnit(static_cast<GLuint>(o), fbo->colorTexture(i));
                            ++o;
                        }

                        shader.setUniform("in_depth", o);
                        glBindTextureUnit(static_cast<GLuint>(o), fbo->depthTexture());
                        ++o;

                        auto lights = m_lights;
                        for (size_t k = 0; k < lights.size(); ++k) {
                            std::string prefix = MAKE_STR("light[" << k << "].");
                            shader.setUniform(prefix + "position_ws", lights[k]->position());
                            shader.setUniform(prefix + "color", lights[k]->color());
                            shader.setUniform(prefix + "matrix", lights[k]->shadowMap().camera().projectionMatrix() * lights[k]->shadowMap().camera().viewMatrix());
                            shader.setUniform(prefix + "isRich", lights[k]->isRich() ? 1 : 0);
                            shader.setUniform(prefix + "hasShadowMap", lights[k]->hasShadowMap() ? 1 : 0);
                            shader.setUniform(prefix + "shadowMap", o);
                            glBindTextureUnit(static_cast<GLuint>(o), lights[k]->shadowMap().depthTexture());
                            ++o;
                            if (lights[k]->isRich()) {
                                shader.setUniform(prefix + "richShadowMap", o);
                                glBindTextureUnit(static_cast<GLuint>(o), lights[k]->shadowMap().colorTexture(0));
                                ++o;
                            }
                        }

                        shader.setUniform("uIsShadowMapEnabled", true);

                        shader.setUniform("uShadowMapBias", 0.00116f);
                        shader.setUniform("uMaxSampleCount", 10.f);

                        shader.setUniform("uHasColormap", static_cast<bool>(defferedShadingColormap));
                        if (colormap) {
                            shader.setUniform("uColormap", static_cast<GLint>(o));
                            glBindTextureUnit(o++, defferedShadingColormap);
                        }
                    }
                    shader.use();
                    glBindVertexArray(defferedVAO);
                    glDrawArrays(GL_POINTS, 0, 1);
                    glBindVertexArray(0);
                }
            }
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
