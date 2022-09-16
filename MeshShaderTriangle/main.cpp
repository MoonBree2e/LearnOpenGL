#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <string_view>

#include <glad/glad.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/ext.hpp>


// Function prototypes
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);

void checkShader(GLuint shader);
void checkProgram(GLuint program);
GLuint createShader(std::string_view filename, GLenum shaderType);
GLuint createProgram(const std::vector<GLuint>& shaders);


int main()
{
	if (!glfwInit())
		return -1;

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	GLFWwindow* window = glfwCreateWindow(1024, 768, "GL_NV_mesh_shader", nullptr, nullptr);
	if (!window)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);

	glfwSetKeyCallback(window, key_callback);


	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize OpenGL context" << std::endl;
		return -1;
	}

	if (!GLAD_GL_NV_mesh_shader)
	{
		std::cout << "GL_NV_mesh_shader not supported!!!" << std::endl;
		return -1;
	}

	if (GLAD_GL_VERSION_4_6) {
		std::cout << "We support at least OpenGL version 4.6" << std::endl;
	}

	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	glViewport(0, 0, width, height);

	auto ms = createShader("ms.mesh", GL_MESH_SHADER_NV);
	auto ps = createShader("ps.frag", GL_FRAGMENT_SHADER);
	auto nv_mesh_prog = createProgram({ ms, ps });

	GLint scale_loc = glGetProgramResourceLocation(nv_mesh_prog, GL_UNIFORM, "scale");

	GLuint vao = 0;
	glCreateVertexArrays(1, &vao);

	{
		GLint x;
		glGetIntegerv(GL_MAX_DRAW_MESH_TASKS_COUNT_NV, &x);
		std::cout << "GL_MAX_DRAW_MESH_TASKS_COUNT_NV: " << x << std::endl;
	}

	{
		GLint x, y, z;
		glGetIntegeri_v(GL_MAX_MESH_WORK_GROUP_SIZE_NV, 0, &x);
		glGetIntegeri_v(GL_MAX_MESH_WORK_GROUP_SIZE_NV, 1, &y);
		glGetIntegeri_v(GL_MAX_MESH_WORK_GROUP_SIZE_NV, 2, &z);
		std::cout << "GL_MAX_MESH_WORK_GROUP_SIZE_NV: " << x << "x" << y << "x" << z << std::endl;
	}

	{
		GLint x;
		glGetIntegerv(GL_MAX_MESH_OUTPUT_VERTICES_NV, &x);
		std::cout << "GL_MAX_MESH_OUTPUT_VERTICES_NV: " << x << std::endl;
	}

	{
		GLint x;
		glGetIntegerv(GL_MAX_MESH_OUTPUT_PRIMITIVES_NV, &x);
		std::cout << "GL_MAX_MESH_OUTPUT_PRIMITIVES_NV: " << x << std::endl;
	}


	while (!glfwWindowShouldClose(window))
	{
		glClearBufferfv(GL_COLOR, 0, &glm::vec4(0.2f, 0.2f, 0.2f, 1.0f)[0]);
		glClearBufferfv(GL_DEPTH, 0, &glm::vec4(1.0f)[0]);

		glUseProgram(nv_mesh_prog);
		glUniform1f(scale_loc, 0.9f);

		glBindVertexArray(vao);
		glDrawMeshTasksNV(0, 1);
		glBindVertexArray(0);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glDeleteProgram(nv_mesh_prog);
	glDeleteVertexArrays(1, &vao);

	glfwTerminate();

	return 0;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
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