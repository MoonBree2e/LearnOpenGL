#pragma once
#ifndef SHADER_H
#define SHADER_H

#include <glad/glad.h>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <variant>

class Shader
{
public:
    unsigned int ID;
    Shader(const char* computePath) {
        // 1. retrieve the vertex/fragment source code from filePath
        std::string computeCode;

        std::ifstream computeFile;

        // ensure ifstream objects can throw exceptions:
        computeFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

        try
        {
            // open files
            computeFile.open(computePath);

            std::stringstream computeStream;
            // read file's buffer contents into streams
            computeStream << computeFile.rdbuf();

            // close file handlers
            computeFile.close();

            // convert stream into string
            computeCode = computeStream.str();
        }
        catch (std::ifstream::failure& e)
        {
            std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ: " << e.what() << std::endl;
        }
        const char* shaderCode = computeCode.c_str();
        // 2. compile shaders
        unsigned int compute;
        // vertex shader
        compute = glCreateShader(GL_COMPUTE_SHADER);
        glShaderSource(compute, 1, &shaderCode, NULL);
        glCompileShader(compute);
        checkCompileErrors(compute, "COMPUTE");
        // shader Program
        ID = glCreateProgram();
        glAttachShader(ID, compute);

        glLinkProgram(ID);
        checkCompileErrors(ID, "PROGRAM");
        // delete the shaders as they're linked into our program now and no longer necessary
        glDeleteShader(compute);

    }

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
        if (geometryPath != nullptr) glAttachShader(ID, geometry);
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
    void unUse() {
        glUseProgram(0);
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
    void setVec4(const std::string& name, glm::vec4 v)
    {
        glUniform4f(glGetUniformLocation(ID, name.c_str()), v.x, v.y, v.z, v.w);
    }
    void setVec2(const std::string& name, float x, float y)
    {
        glUniform2f(glGetUniformLocation(ID, name.c_str()), x, y);
    }
    void setVec2(const std::string& name, glm::vec2 v)
    {
        glUniform2f(glGetUniformLocation(ID, name.c_str()), v.x, v.y);
    }

    bool bindUniformBlock(const std::string& uniformBlockName, GLuint buffer, GLuint uniformBlockBinding = 1) const {
        GLuint index = uniformBlockIndex(uniformBlockName);
        if (index == GL_INVALID_INDEX) {
            // WARN_LOG << "Uniform Block not found with name " << uniformBlockName; // pb: this warning is repeated at each frame, enable verbosity onl right after loading
            return false;
        }
        glBindBufferBase(GL_UNIFORM_BUFFER, uniformBlockBinding, buffer);
        glUniformBlockBinding(ID, index, uniformBlockBinding);
        return true;
    }

    void setUniform(const std::string& vUniformName,
        const std::variant<bool, GLint, GLuint, GLfloat, glm::vec2, glm::vec3, glm::ivec3, glm::mat4>& vValue) const
    {
        const GLint UniformLocation = glGetUniformLocation(ID, vUniformName.c_str());
        if (UniformLocation == -1) {
            printf("[Shader] {%x} SetUniform {%s} failed\n", ID, vUniformName.c_str());
            return;
        }
        struct Visitor
        {
            GLuint _ProgramID;
            GLuint _UniformLocation;
            Visitor(GLuint vProgram, GLint vLocation) : _ProgramID(vProgram), _UniformLocation(vLocation) { }

            void operator()(bool value) { glProgramUniform1i(_ProgramID, _UniformLocation, value); }
            void operator()(GLint value) { glProgramUniform1i(_ProgramID, _UniformLocation, value); }
            void operator()(GLuint value) { glProgramUniform1ui(_ProgramID, _UniformLocation, value); }
            void operator()(GLfloat value) { glProgramUniform1f(_ProgramID, _UniformLocation, value); }
            void operator()(const glm::vec2& value) { glProgramUniform2fv(_ProgramID, _UniformLocation, 1, glm::value_ptr(value)); }
            void operator()(const glm::vec3& value) { glProgramUniform3fv(_ProgramID, _UniformLocation, 1, glm::value_ptr(value)); }
            void operator()(const glm::ivec3& value) { glProgramUniform3iv(_ProgramID, _UniformLocation, 1, glm::value_ptr(value)); }
            void operator()(const glm::mat4& value) { glProgramUniformMatrix4fv(_ProgramID, _UniformLocation, 1, GL_FALSE, glm::value_ptr(value)); }
        };
        std::visit(Visitor{ ID, UniformLocation }, vValue);
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
            m_isValid = success;
        }
    }
    inline GLint uniformLocation(const std::string& name) const { return m_isValid ? glGetUniformLocation(ID, name.c_str()) : GL_INVALID_INDEX; }
    inline GLuint uniformBlockIndex(const std::string& name) const { return m_isValid ? glGetUniformBlockIndex(ID, name.c_str()) : GL_INVALID_INDEX; }
    inline GLuint storageBlockIndex(const std::string& name) const { return m_isValid ? glGetProgramResourceIndex(ID, GL_SHADER_STORAGE_BLOCK, name.c_str()) : GL_INVALID_INDEX; }
    bool m_isValid = false;
};

#endif