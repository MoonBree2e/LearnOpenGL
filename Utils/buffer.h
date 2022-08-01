#pragma once
#include <vector>

#include <glad/glad.h>
#include <spdlog/spdlog.h>

namespace glcs {
    class Buffer
    {
    private:
        GLuint BufferID;
        uint32_t BufferSize;
        std::string Label;
    public:
        Buffer(const std::string& vLabel = "") : BufferID{0}, BufferSize{0}, Label(vLabel)
        {
            glCreateBuffers(1, &BufferID);

            spdlog::info("[Buffer] created buffer {:x}", BufferID);
            if (vLabel != "") glObjectLabel(GL_BUFFER, BufferID, -1, Label.c_str());
        }
        Buffer(const Buffer& buffer) = delete;
        Buffer(Buffer&& vOther) : BufferID(vOther.BufferID), BufferSize(vOther.BufferSize)
        {
            vOther.BufferID = 0;
        }
        Buffer& operator=(const Buffer& buffer) = delete;
        Buffer& operator=(Buffer&& other)
        {
            if (this != &other) {
                release();

                BufferID = other.BufferID;
                BufferSize = other.BufferSize;

                other.BufferID = 0;
            }
            return *this;
        }
        ~Buffer() { release(); }
        
        void release()
        {
            if (BufferID)
            {
                spdlog::info("[Buffer] release buffer {:x}", BufferID);

                glDeleteBuffers(1, &BufferID);
                this->BufferID = 0;
            }
        }
        GLuint getName() const { return BufferID; }
        uint32_t getLength() const { return BufferSize; }

        template <typename T>
        void setStorage(int size)
        {
            glNamedBufferStorage(this->BufferID, size * sizeof(T), NULL, 0);
        }

        template <typename T>
        void setStorage(const std::vector<T>& data, GLenum usage)
        {
            glNamedBufferStorage(this->BufferID, sizeof(T) * data.size(), data.data(), usage);
            this->BufferSize = data.size();
        }

        template <typename T>
        void setData(const std::vector<T>& data, GLenum usage)
        {
            glNamedBufferData(this->BufferID, sizeof(T) * data.size(), data.data(), usage);
            this->BufferSize = data.size();
        }

        template <typename T>
        void clearData(const std::vector<T>& data, GLenum internalformat, GLenum format, GLenum type)
        {
            glClearNamedBufferData(BufferID, internalformat, format, type, data.data());
        }

        void bindToShaderStorageBuffer(GLuint binding_point_index) const
        {
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding_point_index, BufferID);
        }
    };
};