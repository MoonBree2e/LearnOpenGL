#pragma once
#ifndef SHADER_H
#define SHADER_H
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <variant>

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#undef APIENTRY
#include <spdlog/spdlog.h>

class Shader
{
public:
    unsigned int ID;
    // constructor generates the shader on the fly
    // ------------------------------------------------------------------------
    Shader(const char* vertexPath, const char* fragmentPath, const char* geometryPath = nullptr)
    {
        // 1. retrieve the vertex/fragment source code from filePath
        std::string vertexCode;
        std::string fragmentCode;
        std::string geometryCode;
        std::ifstream vShaderFile;
        std::ifstream fShaderFile;
        std::ifstream gShaderFile;
        // ensure ifstream objects can throw exceptions:
        vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        gShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        try
        {
            // open files
            vShaderFile.open(vertexPath);
            fShaderFile.open(fragmentPath);
            std::stringstream vShaderStream, fShaderStream;
            // read file's buffer contents into streams
            vShaderStream << vShaderFile.rdbuf();
            fShaderStream << fShaderFile.rdbuf();
            // close file handlers
            vShaderFile.close();
            fShaderFile.close();
            // convert stream into string
            vertexCode = vShaderStream.str();
            fragmentCode = fShaderStream.str();
            if (geometryPath != nullptr) 
            {
                gShaderFile.open(geometryPath);
                std::stringstream gShaderStream;
                gShaderStream << gShaderFile.rdbuf();
                gShaderFile.close();
                geometryCode = gShaderStream.str();
            }
        }
        catch (std::ifstream::failure& e)
        {
            std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ: " << e.what() << std::endl;
        }
        const char* vShaderCode = vertexCode.c_str();
        const char* fShaderCode = fragmentCode.c_str();
        // 2. compile shaders
        unsigned int vertex, fragment;
        // vertex shader
        vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vShaderCode, NULL);
        glCompileShader(vertex);
        checkCompileErrors(vertex, "VERTEX");
        // fragment Shader
        fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fShaderCode, NULL);
        glCompileShader(fragment);
        checkCompileErrors(fragment, "FRAGMENT");
        // geometry shader
        unsigned int geometry;
        if (geometryPath != nullptr)
        {
            const char* gShaderCode = geometryCode.c_str();
            geometry = glCreateShader(GL_GEOMETRY_SHADER);
            glShaderSource(geometry, 1, &gShaderCode, NULL);
            glCompileShader(geometry);
            checkCompileErrors(geometry, "GEOMETRY");
        }
        // shader Program
        ID = glCreateProgram();
        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);
        if(geometryPath != nullptr) glAttachShader(ID, geometry);
        glLinkProgram(ID);
        checkCompileErrors(ID, "PROGRAM");
        // delete the shaders as they're linked into our program now and no longer necessary
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        if (geometryPath != nullptr) glDeleteShader(geometry);
    }
    // activate the shader
    // ------------------------------------------------------------------------
    void use()
    {
        glUseProgram(ID);
    }
    // utility uniform functions
    // ------------------------------------------------------------------------
    void setBool(const std::string& name, bool value) const
    {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
    }
    // ------------------------------------------------------------------------
    void setInt(const std::string& name, int value) const
    {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
    }
    // ------------------------------------------------------------------------
    void setFloat(const std::string& name, float value) const
    {
        glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
    }
    // ------------------------------------------------------------------------
    void setMat4(const std::string& name, glm::mat4 mat) const
    {
        glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, glm::value_ptr(mat));
    }
    void setVec3(const std::string& name, float x, float y, float z) 
    {
        glUniform3f(glGetUniformLocation(ID, name.c_str()), x, y, z);
    }
    void setVec3(const std::string& name, glm::vec3 v)
    {
        glUniform3f(glGetUniformLocation(ID, name.c_str()), v.x, v.y, v.z);
    }
    void setVec2(const std::string& name, float x, float y)
    {
        glUniform2f(glGetUniformLocation(ID, name.c_str()), x, y);
    }
    void setVec2(const std::string& name, glm::vec2 v)
    {
        glUniform2f(glGetUniformLocation(ID, name.c_str()), v.x, v.y);
    }


private:
    // utility function for checking shader compilation/linking errors.
    // ------------------------------------------------------------------------
    void checkCompileErrors(unsigned int shader, std::string type)
    {
        int success;
        char infoLog[1024];
        if (type != "PROGRAM")
        {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success)
            {
                glGetShaderInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        }
        else
        {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if (!success)
            {
                glGetProgramInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        }
    }
};


namespace glcs
{
    class Shader
    {
    private:
        GLuint ProgramID;

        static std::string loadStringFromFile(const std::filesystem::path& vFilePath)
        {
            std::ifstream File(vFilePath);
            if (!File.is_open()) {
                spdlog::error("[Shader] failed to open {}", vFilePath.generic_string());
                std::exit(EXIT_FAILURE);
            }
            return std::string(std::istreambuf_iterator<char>(File), std::istreambuf_iterator<char>());
        }
    public:
        Shader(GLenum vType, const std::filesystem::path& vFilePath)
        {
            const std::string ShaderSourceCode = loadStringFromFile(vFilePath);
            const char* ShaderSourceCodeC = ShaderSourceCode.c_str();
            ProgramID = glCreateShaderProgramv(vType, 1, &ShaderSourceCodeC);
            spdlog::info("[Shader] program {:x} created", ProgramID);

            // check compile and link error
            int Success = false;
            glGetProgramiv(ProgramID, GL_LINK_STATUS, &Success);
            if (Success == GL_FALSE)
            {
                spdlog::error("[Shader] failed to link program {:x}", ProgramID);

                GLint LogSize = 0;
                glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &LogSize);
                std::vector<GLchar> ErrorLog(LogSize);
                glGetProgramInfoLog(ProgramID, LogSize, &LogSize, &ErrorLog[0]);
                std::string ErrorLogStr(ErrorLog.begin(), ErrorLog.end());
                spdlog::error("[Shader] {}", ErrorLogStr);
            }
        }
        Shader(const Shader& vOther) = delete;
        Shader(Shader& vOther) : ProgramID(vOther.ProgramID) { vOther.ProgramID = 0; }
        Shader& operator=(const Shader& vOther) = delete;
        Shader& operator=(Shader&& vOther) 
        {
            if (this != &vOther)
            {
                release();
                ProgramID = vOther.ProgramID;
                vOther.ProgramID = 0;
            }
            return *this;
        }

        virtual ~Shader() { release(); }

        void release()
        {
            if (ProgramID) {
                spdlog::info("[Shader] release program {:x}", ProgramID);
                glDeleteProgram(ProgramID);
            }
        }
        
        GLuint getProgram() const { return ProgramID; }
        void setUniform(const std::string& vUniformName,
                        const std::variant<bool, GLint, GLuint, GLfloat, glm::vec2, glm::vec3, glm::ivec3, glm::mat4>& vValue) const
        {
            const GLint UniformLocation = glGetUniformLocation(ProgramID, vUniformName.c_str());
            if (UniformLocation == -1) {
                spdlog::error("[Shader] {:x} SetUniform {:s} failed", ProgramID, vUniformName.c_str());
            }
            struct Visitor
            {
                GLuint _ProgramID;
                GLuint _UniformLocation;
                Visitor(GLuint vProgram, GLint vLocation) : _ProgramID(vProgram), _UniformLocation(vLocation) { }
                
                void operator()(bool value)   { glProgramUniform1i(_ProgramID, _UniformLocation, value); }
                void operator()(GLint value)  { glProgramUniform1i(_ProgramID, _UniformLocation, value); }
                void operator()(GLuint value) { glProgramUniform1ui(_ProgramID, _UniformLocation, value); }
                void operator()(GLfloat value){ glProgramUniform1f(_ProgramID, _UniformLocation, value); }
                void operator()(const glm::vec2& value) { glProgramUniform2fv(_ProgramID, _UniformLocation, 1, glm::value_ptr(value)); }
                void operator()(const glm::vec3& value) { glProgramUniform3fv(_ProgramID, _UniformLocation, 1, glm::value_ptr(value)); }
                void operator()(const glm::ivec3& value){ glProgramUniform3iv(_ProgramID, _UniformLocation, 1, glm::value_ptr(value)); }
                void operator()(const glm::mat4& value) { glProgramUniformMatrix4fv(_ProgramID, _UniformLocation, 1, GL_FALSE, glm::value_ptr(value)); }
            };
            std::visit(Visitor{ ProgramID, UniformLocation }, vValue);
        }
    };

    class VertexShader : public Shader
    {
    public:
        VertexShader(const std::filesystem::path& vFilePath) :Shader(GL_VERTEX_SHADER, vFilePath) {}
    };

    class GeometryShader : public Shader
    {
    public:
        GeometryShader(const std::filesystem::path& vFilePath) :Shader(GL_GEOMETRY_SHADER, vFilePath) {}
    };

    class FragmentShader : public Shader
    {
    public:
        FragmentShader(const std::filesystem::path& vFilePath) :Shader(GL_FRAGMENT_SHADER, vFilePath) {}
    };

    class ComputeShader : public Shader
    {
    public:
        ComputeShader(const std::filesystem::path& vFilePath) :Shader(GL_COMPUTE_SHADER, vFilePath) {}
    };

    class Pipeline
    {
    public:
        GLuint PipelineID;
         
        Pipeline()
        {
            glCreateProgramPipelines(1, &PipelineID);
            spdlog::info("[Pipeline] pipeline {:x} created", PipelineID);
        }
        Pipeline(const Pipeline& other) = delete;
        Pipeline(Pipeline&& other) : PipelineID(other.PipelineID) { other.PipelineID = 0; }
        Pipeline& operator=(const Pipeline& vOther) = delete;
        Pipeline& operator=(Pipeline&& vOther)
        {
            if (this != &vOther) {
                release();
                PipelineID = vOther.PipelineID;
                vOther.PipelineID = 0;
            }
            return *this;
        }

        ~Pipeline() { release(); }
        void release()
        {
            if (PipelineID) {
                spdlog::info("[Pipeline] release pipeline {:x}", PipelineID);
                glDeleteProgramPipelines(1, &PipelineID);
            }
        }

        void attachVertexShader(const VertexShader& vShader) const
        {
            glUseProgramStages(PipelineID, GL_VERTEX_SHADER_BIT, vShader.getProgram());
        }

        void attachFragmentShader(const FragmentShader& vShader) const {
            glUseProgramStages(PipelineID, GL_FRAGMENT_SHADER_BIT, vShader.getProgram());
        }

        void attachComputeShader(const ComputeShader& vShader) const {
            glUseProgramStages(PipelineID, GL_COMPUTE_SHADER_BIT, vShader.getProgram());
        }


        void activate() const { glBindProgramPipeline(PipelineID); }
        void deactivate() const { glBindProgramPipeline(0); }
    };

}
#endif