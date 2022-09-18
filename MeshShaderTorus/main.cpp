
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define _USE_MATH_DEFINES
#include <math.h>
#include <cstdio>
#include <numeric>

#include "shader.h"
#include "camera.h"
#include "buffer.h"
#include "vertexArrayObject.h"

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

glm::vec3 lightPos(1.2f, 1.f, 2.f);
Camera camera(glm::vec3(0.0f, 0.0f, 5.0f));

float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

int RenderMode = 1;

int meshlet_vertices = 64;
int meshlet_triangles = 16;
int meshlet_indices = meshlet_triangles * 3;

struct uniformMat
{
    glm::mat4 ViewProjectionMatrix;
    glm::mat4 ModelMatrix;
};

struct uniformVertex
{
    glm::vec4 position;
    glm::vec4 color;
};

struct uniformMeshlet
{
    GLuint vertices[64];
    GLuint indices[378]; // up to 126 triangles
    GLuint vertex_count;
    GLuint index_count;
};

void processInput(GLFWwindow* window);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);

void checkShader(GLuint shader);
void checkProgram(GLuint program);
GLuint createShader(std::string_view filename, GLenum shaderType);
GLuint createProgram(const std::vector<GLuint>& shaders);

void createTorus(std::vector<float>& vTorus, float _R, float _r, int _nR, int _nr);

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Mesh", NULL, NULL);
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        return -1;
    }

    
    std::vector<float> TorusVertices;
    createTorus(TorusVertices, 1, 0.35, 32, 32);
    int VerticesCount = TorusVertices.size()/3;
    std::vector<GLuint> TorusIndices(VerticesCount);
    std::iota(TorusIndices.begin(), TorusIndices.end(),0);


    unsigned VBO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, TorusVertices.size()*4, TorusVertices.data(), GL_STATIC_DRAW);
    unsigned EBO;
    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * TorusIndices.size(), TorusIndices.data(), GL_STATIC_DRAW);

    //-------------------------------------
    // vertex shader
    //
    GLuint VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    Shader torusShader("vs.vert", "fs.frag");

    //-------------------------------------
    // mesh shader
    //
    auto ms = createShader("ms.mesh", GL_MESH_SHADER_NV);
    auto fs = createShader("fs.frag", GL_FRAGMENT_SHADER);
    auto meshProg = createProgram({ ms, fs });

    //-------------------------------------
    // transform
    //
    GLuint transformUbIndex = glGetUniformBlockIndex(meshProg, "uniforms_t");
    glUniformBlockBinding(meshProg, transformUbIndex, 0);
    GLuint transformUb;
    glGenBuffers(1, &transformUb);
    glBindBuffer(GL_UNIFORM_BUFFER, transformUb);
    glBufferData(GL_UNIFORM_BUFFER, 2 * sizeof(glm::mat4), NULL, GL_STATIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    // define the range of the buffer that links to a uniform binding point
    glBindBufferRange(GL_UNIFORM_BUFFER, 0, transformUb, 0, 2 * sizeof(glm::mat4));

    //-------------------------------------
    // vertex
    //
    std::vector<uniformVertex> uniformVertexData;
    for (int i = 0; i < TorusVertices.size()/3; ++i)
        uniformVertexData.push_back({ glm::vec4(TorusVertices[i],TorusVertices[i + 1],TorusVertices[i + 2], 1), 
                                      glm::vec4(1,1,1,1)});
    VertexArrayObject vertexVAO;
    Buffer vertexBuffer;
    vertexBuffer.setData(uniformVertexData, GL_DYNAMIC_DRAW);
    vertexVAO.bindVertexBuffer(vertexBuffer,0,0,sizeof(uniformVertex));
    vertexVAO.activateVertexAttribution(0, 0, 4, GL_FLOAT, 0);
    vertexVAO.activateVertexAttribution(0, 1, 4, GL_FLOAT, 4 * sizeof(GL_FLOAT));

    //-------------------------------------
    // meshlets
    //
    std::vector<uniformMeshlet> uniformMeshletData;

    glEnable(GL_DEPTH_TEST);

    while (!glfwWindowShouldClose(window))
    {
        processInput(window);

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 view = camera.getViewMatrix();
        glm::mat4 projection = glm::perspective(glm::radians(camera.Fov), (float)SCR_WIDTH / (float)SCR_HEIGHT, .1f, 100.f);

        glBindBuffer(GL_UNIFORM_BUFFER, transformUb);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), glm::value_ptr(projection));
        glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4), glm::value_ptr(view));
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        if (RenderMode == 1) {
            torusShader.use();
            torusShader.setMat4("model", model);
            glBindVertexArray(VAO);
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glDrawElements(GL_TRIANGLE_STRIP, TorusIndices.size(), GL_UNSIGNED_INT, 0);
            //glDrawArrays(GL_TRIANGLE_STRIP, 0, VerticesCount);
        }
        else if(RenderMode == 2) {

        }


        glfwSwapBuffers(window);
        glfwPollEvents();
    }

}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    float currentFrame = static_cast<float>(glfwGetTime());
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.processKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.processKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.processKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.processKeyboard(RIGHT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
        RenderMode = 1;
    if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
        RenderMode = 2;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
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

    camera.processMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.processMouseScroll(static_cast<float>(yoffset));
}

GLuint createShader(std::string_view filename, GLenum shaderType)
{
    auto source = [filename]() {
        std::string result;
        std::ifstream stream(filename.data());

        if (!stream.is_open()) {
            std::cerr << "Could not open file: " << std::string(filename) << '\n';
            return result;
        }

        stream.seekg(0, std::ios::end);
        result.reserve((size_t)stream.tellg());
        stream.seekg(0, std::ios::beg);

        result.assign(std::istreambuf_iterator<char>{stream},
            std::istreambuf_iterator<char>{});

        return result;
    }();

    auto pSource = source.c_str();

    GLuint shader{ glCreateShader(shaderType) };
    glShaderSource(shader, 1, &pSource, nullptr);
    glCompileShader(shader);
    checkShader(shader);

    return shader;
}

GLuint createProgram(const std::vector<GLuint>& shaders)
{
    GLuint program{ glCreateProgram() };

    for (const auto& shader : shaders)
    {
        glAttachShader(program, shader);
    }

    glLinkProgram(program);
    checkProgram(program);

    for (const auto& shader : shaders)
    {
        glDetachShader(program, shader);
        glDeleteShader(shader);
    }

    return program;
}

void checkShader(GLuint shader)
{
    GLint isCompiled = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);

    if (isCompiled == GL_FALSE)
    {
        GLint maxLength{};
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

        if (maxLength > 0)
        {
            std::vector<char> errorLog(maxLength);
            glGetShaderInfoLog(shader, maxLength, nullptr, errorLog.data());
            glDeleteShader(shader);

            std::cerr << "Error compiled:\n" << errorLog.data() << '\n';
        }
    }
}

void checkProgram(GLuint program)
{
    GLint isLinked = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &isLinked);

    if (isLinked == GL_FALSE)
    {
        GLint maxLength{};
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);

        if (maxLength > 0)
        {
            std::vector<char> errorLog(maxLength);
            glGetProgramInfoLog(program, maxLength, nullptr, errorLog.data());
            glDeleteProgram(program);

            std::cerr << "Error linking:\n" << errorLog.data() << '\n';
        }
    }
}

void createTorus(std::vector<float>& vTorus, float _R, float _r, int _nR, int _nr) {
    float du = 2 * M_PI / _nR;
    float dv = 2 * M_PI / _nr;

    for (size_t i = 0; i < _nR; i++) {

        float u = i * du;

        for (size_t j = 0; j <= _nr; j++) {

            float v = (j % _nr) * dv;

            for (size_t k = 0; k < 2; k++)
            {
                float uu = u + k * du;
                // compute vertex
                float x = (_R + _r * cos(v)) * cos(uu);
                float y = (_R + _r * cos(v)) * sin(uu);
                float z = _r * sin(v);

                // add vertex 
                vTorus.push_back(x);
                vTorus.push_back(y);
                vTorus.push_back(z);
            }
        }
    }
}