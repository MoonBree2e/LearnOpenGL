#pragma once

#include <glad/glad.h>
#include <spdlog/spdlog.h>

#include "buffer.h"

namespace glcs {
    class VertexArrayObject
    {
    private:
        GLuint ArrayID;

    public:
        VertexArrayObject() { glCreateVertexArrays(1, &ArrayID); }
        VertexArrayObject(const VertexArrayObject& other) = delete;
        VertexArrayObject(VertexArrayObject&& other) :ArrayID(other.ArrayID){ other.ArrayID = 0; }
        VertexArrayObject& operator=(const VertexArrayObject& other) = delete;
        VertexArrayObject& operator=(VertexArrayObject&& other){
            if (this != &other) {
                release();
                ArrayID = other.ArrayID;
                other.ArrayID = 0;
            }
            return *this;
        }

        void bindVertexBuffer(const Buffer& buffer, GLuint binding, GLintptr offset, GLsizei stride) const
        {
            glVertexArrayVertexBuffer(ArrayID, binding, buffer.getName(), offset, stride);
        }

        void bindElementBuffer(const Buffer& buffer) const
        {
            glVertexArrayElementBuffer(ArrayID, buffer.getName());
        }

        void activateVertexAttribution(GLuint binding, GLuint attrib, GLint size, GLenum type, GLsizei offset) const
        {
            glEnableVertexArrayAttrib(ArrayID, attrib);
            glVertexArrayAttribBinding(ArrayID, attrib, binding);
            glVertexArrayAttribFormat(ArrayID, attrib, size, type, GL_FALSE, offset);
        }

        void activate() const { glBindVertexArray(ArrayID); }

        void deactivate() const { glBindVertexArray(0); }


        void release(){
            if (ArrayID) {
                spdlog::info("[VertexArrayObject release VAO {:x}", ArrayID);
                glDeleteVertexArrays(1, &ArrayID);
            }
        }
    };
}