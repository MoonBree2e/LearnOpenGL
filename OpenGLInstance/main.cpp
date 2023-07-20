#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>

#include <glad/glad.h> 
#include <GLFW/glfw3.h>

#include "camera.h"
#include "shader.h"

#include <random>
#include "stb_image.h"
#include "Coordination.h"

const unsigned int SCR_WIDTH = 1200;
const unsigned int SCR_HEIGHT = 900;
#define CLEAR_DEPTH_VALUE -1000000.0f


void processInput(GLFWwindow* window);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);

int pos = 0;



Camera camera(glm::vec3(0.0f, 0.0f, 5.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

float deltaTime = 0.0f;
float lastFrame = 0.0f;
float cameraSpeedScale = 0.5f;

int main()
{
    glm::dvec3 GlobalPosition;
    g_coord.LongLat2GlobalCoord(120.8334936651, 24.68150604065281, 0.5195652637630701, GlobalPosition);
    glm::vec3 earthPos = glm::vec3(GlobalPosition);
    earthPos = glm::vec3(0);
    camera = Camera(glm::vec3(0.0f, 0.0f, 5.0f) + earthPos);

#pragma region BEGIN
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    const char* glsl_version = "#version 430";

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL) {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
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
#pragma endregion

#pragma region plane
float planeVert[] = {
        1.f,  0.f, -1.0f,   1.0f, 1.0f, 1.0f,   1.0f, 1.0f,   // 右上
        1.f, 0.f, 1.f,   1.0f, 1.0f, 1.0f,   1.0f, 0.0f,   // 右下
        -1.f, 0.f, 1.0f,   1.0f, 1.0f, 1.0f,   0.0f, 0.0f,   // 左下
        -1.f,  0.f, -1.0f,   1.0f, 1.0f, 1.0f,   0.0f, 1.0f    // 左上
    };
    unsigned int indices[] = { // 注意索引从0开始! 
        0, 1, 3, // 第一个三角形
        1, 2, 3  // 第二个三角形
    };
    // ---------load texture---------
    int width1, height1, nrChannels1;
    stbi_set_flip_vertically_on_load(true); // tell stb_image.h to flip loaded texture's on the y-axis.
    unsigned char* data1 = stbi_load("../Assets/container.jpg", &width1, &height1, &nrChannels1, 0);
    unsigned int planeTexture;
    glGenTextures(1, &planeTexture);
    glBindTexture(GL_TEXTURE_2D, planeTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width1, height1, 0, GL_RGB, GL_UNSIGNED_BYTE, data1);
    glGenerateMipmap(GL_TEXTURE_2D);

    unsigned int planeVAO, planeVBO, planeEBO;
    glGenVertexArrays(1, &planeVAO);
    glGenBuffers(1, &planeVBO);
    glGenBuffers(1, &planeEBO);

    glBindVertexArray(planeVAO);

    glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(planeVert), planeVert, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, planeEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    stbi_image_free(data1);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
#pragma endregion


    std::vector<float> vertices;
    int sizeXYZ = 20;
    int nParticles = sizeXYZ * sizeXYZ * sizeXYZ;
    glm::dvec3 corner = glm::dvec3(0) /*+ glm::dvec3(earthPos)*/;
    const double step = 1.0f / static_cast<float>(sizeXYZ - 1);

    float pointSize = step;
    float pointScale = 100/step;
    srand(time(0));
    for (int i = 0; i < sizeXYZ; ++i) {
        for (int j = 0; j < sizeXYZ; ++j) {
            for (int k = 0; k < sizeXYZ; ++k) {
                auto pos = corner + glm::dvec3(i, j, k) * step;
                //NumberHelpers::jitter(pos, randomness);
                auto dirpos = (camera.getViewMatrix() * glm::dvec4(pos, 1.0));
                vertices.push_back(pos.x);
                vertices.push_back(pos.y);
                vertices.push_back(pos.z);
            }
        }
    }

    float densityLowerBound = 1.0f / (8.0f * pow(pointSize, 3.0f)) * 0.001f;

    
    unsigned VBO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vertices.size(), vertices.data(), GL_STATIC_DRAW);

    unsigned VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    unsigned int backgroundFBO;
    glGenFramebuffers(1, &backgroundFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, backgroundFBO);
    unsigned int backgroundTexture;
    glGenTextures(1, &backgroundTexture);
    glBindTexture(GL_TEXTURE_2D, backgroundTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, backgroundTexture, 0);

    unsigned int backgroundDepthTexture;
    glGenTextures(1, &backgroundDepthTexture);
    glBindTexture(GL_TEXTURE_2D, backgroundDepthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, backgroundDepthTexture, 0);


    unsigned int backgroundRBO;
    glGenRenderbuffers(1, &backgroundRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, backgroundRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, SCR_WIDTH, SCR_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, backgroundRBO);
    GLuint attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, attachments);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    unsigned int depthFBO;
    glGenFramebuffers(1, &depthFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, depthFBO);
    unsigned int depthTexture;
    glGenTextures(1, &depthTexture);
    glBindTexture(GL_TEXTURE_2D, depthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, depthTexture, 0);
    unsigned int depthRBO;
    glGenRenderbuffers(1, &depthRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, depthRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, SCR_WIDTH, SCR_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthRBO);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    unsigned int thicknessFBO;
    glGenFramebuffers(1, &thicknessFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, thicknessFBO);
    unsigned int thicknessTexture;
    glGenTextures(1, &thicknessTexture);
    glBindTexture(GL_TEXTURE_2D, thicknessTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, thicknessTexture, 0);
    unsigned int thicknessRBO;
    glGenRenderbuffers(1, &thicknessRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, thicknessRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, SCR_WIDTH, SCR_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, thicknessRBO);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    Shader planeShader("plane.vert", "plane.frag");
    Shader shaderParticles("particle.vert", "particle.frag");   
    Shader depthShader("depth.vert", "depth.frag");
    Shader thicknessShader("thickness.vert", "thickness.frag");
    Shader normalShader("quad.vert", "normal.frag");

    bool drawParticles = false;
    bool drawDepth = true;
    while (!glfwWindowShouldClose(window))
    {
        processInput(window);

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 view = camera.getViewMatrix();
        glm::mat4 projection = glm::perspective(glm::radians(camera.Fov), (float)SCR_WIDTH / (float)SCR_HEIGHT, .1f, 100.f);
        
        // --------- background --------
        {
            glBindFramebuffer(GL_FRAMEBUFFER, backgroundFBO);
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            model = glm::translate(model, earthPos);
            model = glm::translate(model, glm::vec3(0,-1,0));
            model = glm::scale(model, glm::vec3(2, 1, 1));
            planeShader.use();
            planeShader.setMat4("modelMatrix", model);
            planeShader.setMat4("viewMatrix", view);
            planeShader.setMat4("projectMatrix", projection);
            planeShader.setInt("tex", 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, planeTexture);
            glBindVertexArray(planeVAO);
            glEnable(GL_DEPTH_TEST);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

            model = glm::mat4(1.0f);
            model = glm::translate(model, earthPos);
            model = glm::translate(model, glm::vec3(0, 0, -1));
            model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0, 0.0, 0.0));
            model = glm::scale(model, glm::vec3(2, 1, 1));
            planeShader.setMat4("modelMatrix", model);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

            model = glm::mat4(1.0f);
            model = glm::translate(model, earthPos);
            model = glm::translate(model, glm::vec3(-2, 0, 0));
            model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0, 1.0, 0.0));
            model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0, 0.0, 0.0));
            planeShader.setMat4("modelMatrix", model);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

            planeShader.unUse();
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }
#if 1
        if (drawParticles) {
            shaderParticles.use();
            shaderParticles.setMat4("modelMatrix", model);
            shaderParticles.setMat4("viewMatrix", view);
            shaderParticles.setMat4("projectMatrix", projection);
            //shaderParticles.setVec3("viewPos", camera.Position);
            shaderParticles.setFloat("pointScale", pointScale);
            shaderParticles.setFloat("pointSize", pointSize);
            shaderParticles.setInt("u_nParticles", sizeXYZ * sizeXYZ * sizeXYZ);
            //shaderParticles.setFloat("time", glfwGetTime());

            glBindVertexArray(VAO);
            glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
            glEnable(GL_DEPTH_TEST);
            glDrawArrays(GL_POINTS, 0, nParticles);
            shaderParticles.unUse();
        }
        // --------- fluid depth ---------
        {
            glBindFramebuffer(GL_FRAMEBUFFER, depthFBO);
            glClearColor(CLEAR_DEPTH_VALUE, CLEAR_DEPTH_VALUE, CLEAR_DEPTH_VALUE, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            depthShader.use();
            depthShader.setMat4("modelMatrix", model);
            depthShader.setMat4("viewMatrix", view);
            depthShader.setMat4("projectMatrix", projection);
            depthShader.setFloat("pointScale", pointScale);
            depthShader.setFloat("pointSize", pointSize);
            depthShader.setFloat("densityLowerBound", densityLowerBound);
            glBindVertexArray(VAO);
            glEnable(GL_DEPTH_TEST);
            glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
            glDrawArrays(GL_POINTS, 0, nParticles);
            depthShader.unUse();
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }
        // --------- fluid thickness ---------
        {
            glBindFramebuffer(GL_FRAMEBUFFER, thicknessFBO);
            glClearColor(0, 0, 0, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            thicknessShader.use();
            thicknessShader.setMat4("modelMatrix", model);
            thicknessShader.setMat4("viewMatrix", view);
            thicknessShader.setMat4("projectMatrix", projection);
            thicknessShader.setFloat("pointScale", pointScale * 2.f);
            thicknessShader.setFloat("pointSize", pointSize);
            glEnable(GL_BLEND);
            glBlendFunc(GL_ONE, GL_ONE);
            glDepthMask(GL_FALSE);
            glEnable(GL_DEPTH_TEST);
            glDisable(GL_DEPTH_TEST);
            glEnable(GL_PROGRAM_POINT_SIZE);
            glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
            glBindVertexArray(VAO);
            glDrawArrays(GL_POINTS, 0, nParticles);

            glDepthMask(GL_TRUE);
            glDisable(GL_BLEND);
            glDisable(GL_PROGRAM_POINT_SIZE);

            thicknessShader.unUse();
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }
        // --------- fluid normal and rendering ---------
        {
            int tex = 0;
            normalShader.use();
            glActiveTexture(GL_TEXTURE0 + tex);
            glBindTexture(GL_TEXTURE_2D, depthTexture);
            normalShader.setInt("depthTex", tex++);

            glActiveTexture(GL_TEXTURE0 + tex);
            glBindTexture(GL_TEXTURE_2D, thicknessTexture);
            normalShader.setInt("thicknessTex", tex++);

            glActiveTexture(GL_TEXTURE0 + tex);
            glBindTexture(GL_TEXTURE_2D, backgroundTexture);
            normalShader.setInt("backgroundTex", tex++);

            glActiveTexture(GL_TEXTURE0 + tex);
            glBindTexture(GL_TEXTURE_2D, backgroundDepthTexture);
            normalShader.setInt("backgroundDepthTex", tex++);

            normalShader.setFloat("pointSize", pointSize);
            normalShader.setMat4("viewMatrix", view);
            normalShader.setMat4("invViewMatrix", glm::inverse(view));
            normalShader.setMat4("projectMatrix", projection);
            normalShader.setVec4("liquidColor", glm::vec4(.275f, 0.65f, 0.85f, 0.5f));
            normalShader.setMat4("invProjectMatrix", glm::inverse(projection));
            normalShader.setVec3("cameraPos", camera.Position);

            glDrawArrays(GL_TRIANGLES, 0, 6);

        }
#endif



        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (show_another_window)
        {
            ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
            ImGui::Text("Hello from another window!");
            ImGui::DragFloat("pointScale", &pointScale, 10);
            ImGui::DragFloat("pointSize", &pointSize, 0.0005f);

            auto desiredWidth = ImGui::GetContentRegionAvail().x;
            auto scale = desiredWidth / SCR_WIDTH;
            auto imageDrawSize = ImVec2(desiredWidth, SCR_HEIGHT * scale);
            auto drawImage = [&](const char* name, GLuint tex) {
                ImGui::Text(name);
                ImGui::Image(reinterpret_cast<ImTextureID>(tex), imageDrawSize, { 0, 1 }, { 1, 0 });
            };
            ImGui::Text("aaaa");
            if(ImGui::CollapsingHeader("depth")) drawImage("depth", depthTexture);
            if (ImGui::CollapsingHeader("thickness")) drawImage("thickness", thicknessTexture);

            ImGui::End();
        }

        // Rendering
        ImGui::Render();

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
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
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.processKeyboard(FORWARD, t);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.processKeyboard(BACKWARD, t);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.processKeyboard(LEFT, t);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.processKeyboard(RIGHT, t);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}


void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;
    if(glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_2) == GLFW_PRESS)
        camera.processMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.processMouseScroll(static_cast<float>(yoffset));
}