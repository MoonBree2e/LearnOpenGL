#include <glad/glad.h>
#include <GLFW/glfw3.h>

#undef APIENTRY
#include <spdlog/spdlog.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <string>

#include "fluid.h"

using namespace glcs;

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

float lastX = (float)SCR_WIDTH / 2.0;
float lastY = (float)SCR_HEIGHT / 2.0;
bool firstMouse = true;

FluidRef g_FluidPtr;



static void framebufferSizeCallback([[maybe_unused]] GLFWwindow* window,
    int width, int height) {
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow* window, const ImGuiIO& io);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "sph", nullptr, nullptr);
    if (!window) return -1;
    glfwMakeContextCurrent(window);

    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    //glfwSwapInterval(0);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        return -1;
    }

    g_FluidPtr = Fluid::create("fluid");
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;

    // set imgui style
    ImGui::StyleColorsDark();

    // init imgui backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 460 core");

    while (!glfwWindowShouldClose(window))
    {
        processInput(window, io);

        // Start Imgui Frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("UI");
        {
            ImGui::Text("Framerate %.3f", io.Framerate);
            ImGui::Separator();

            static int NParticles = g_FluidPtr->getNParticles();
            if (ImGui::InputInt("Number of particles", &NParticles)) {
                NParticles = std::clamp(NParticles, 0, 10000000);
                g_FluidPtr->setNParticles(NParticles);
            }
            //static glm::vec3 BaseColor = g_RenderPtr->getBaseColor();
            //if (ImGui::ColorPicker3("BaseColor", glm::value_ptr(BaseColor))) {
            //    g_RenderPtr->setBaseColor(BaseColor);
            //}
        }
        ImGui::End();

        // update
        g_FluidPtr->update(io.DeltaTime);

        // Render
        g_FluidPtr->draw();

        // RenderImGui
        glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "ImGui Render");
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glPopDebugGroup();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    g_FluidPtr = NULL;
    return 0;
}


void processInput(GLFWwindow* window, const ImGuiIO& io)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if(glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    else glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        g_FluidPtr->processKeyboard(FORWARD, io.DeltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        g_FluidPtr->processKeyboard(BACKWARD, io.DeltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        g_FluidPtr->processKeyboard(LEFT, io.DeltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        g_FluidPtr->processKeyboard(RIGHT, io.DeltaTime);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
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

    g_FluidPtr->processMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    g_FluidPtr->processMouseScroll(static_cast<float>(yoffset));
}