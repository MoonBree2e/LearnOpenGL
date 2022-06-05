#pragma once
#ifndef CAMERA_H
#define CAMERA_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>

enum CameraMoveDirection {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT
};

const float YAW = -90.f;
const float PITCH = 0.f;
const float CAMERASPEED = 5.5f;
const float SENSITIVITY = 0.1f;
const float FOV = 45.f;

class Camera {
public:
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;

    float Yaw;
    float Pitch;

    float MovementSpeed;
    float MouseSensitivity;
    float Fov;
    Camera(glm::vec3 pos = glm::vec3(0.f, 0.f, 0.f), glm::vec3 up = glm::vec3(0.f, 1.f, 0.f), float yaw = YAW, float pitch = PITCH) :
        Front(glm::vec3(0.f, 0.f, -1.f)), Position(pos), WorldUp(up),
        Yaw(yaw), Pitch(pitch), MovementSpeed(CAMERASPEED), MouseSensitivity(SENSITIVITY), Fov(FOV)
    {
        __updateCameraVectors();
    }
    Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch) :
        Front(glm::vec3(0.f, 0.f, -1.f)), Position(glm::vec3(posX, posY, posZ)), WorldUp(glm::vec3(upX, upY, upZ)),
        Yaw(yaw), Pitch(pitch), MovementSpeed(CAMERASPEED), MouseSensitivity(SENSITIVITY), Fov(FOV)
    {
        __updateCameraVectors();
    }

    glm::mat4 getViewMatrix() {
        return glm::lookAt(Position, Position + Front, Up);
    }

    void processKeyboard(CameraMoveDirection vDirection, float vDeltaTime) {
        float Velocity = MovementSpeed * vDeltaTime;
        switch (vDirection)
        {
        case FORWARD:
            Position += Front * Velocity; break;
        case BACKWARD:
            Position -= Front * Velocity; break;
        case LEFT:
            Position -= Right * Velocity; break;
        case  RIGHT:
            Position += Right * Velocity; break;
        default:
            break;
        }
    }

    void processMouseMovement(float vXoffset, float vYoffset, GLboolean constrainPitch = true)
    {
        vXoffset *= MouseSensitivity;
        vYoffset *= MouseSensitivity;

        Yaw += vXoffset;
        Pitch += vYoffset;

        if (constrainPitch)
        {
            if (Pitch > 89.f) Pitch = 89.f;
            if (Pitch < -89.f) Pitch = -89.f;
        }
        __updateCameraVectors();
    }

    void processMouseScroll(float vYoffset)
    {
        Fov -= (float)vYoffset;
        if (Fov < 1.f) Fov = 1.f;
        if (Fov > 55.f) Fov = 55.f;
    }

private:
    void __updateCameraVectors() {
        glm::vec3 front;
        front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        front.y = sin(glm::radians(Pitch));
        front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        Front = glm::normalize(front);
        Right = glm::normalize(glm::cross(Front, WorldUp));
        Up = glm::normalize(glm::cross(Right, Front));
    }
};
#endif