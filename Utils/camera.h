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

const double YAW = -90.f;
const double PITCH = 0.f;
const double CAMERASPEED = 5.5f;
const double SENSITIVITY = 0.1f;
const double FOV = 24.f;

class Camera {
public:
    glm::dvec3 Position;
    glm::dvec3 Front;
    glm::dvec3 Up;
    glm::dvec3 Right;
    glm::dvec3 WorldUp;

    double Yaw;
    double Pitch;

    double MovementSpeed;
    double MouseSensitivity;
    double Fov;

    double ZNear = 0.1f;
    double ZFar = 1000.0f;
    Camera(glm::dvec3 pos = glm::dvec3(0.f, 0.f, 0.f), glm::dvec3 up = glm::dvec3(0.f, 1.f, 0.f), double yaw = YAW, double pitch = PITCH) :
        Front(glm::dvec3(0.f, 0.f, 1.f)), Position(pos), WorldUp(up),
        Yaw(yaw), Pitch(pitch), MovementSpeed(CAMERASPEED), MouseSensitivity(SENSITIVITY), Fov(FOV)
    {
        __updateCameraVectors();
    }
    Camera(double posX, double posY, double posZ, double upX, double upY, double upZ, double yaw, double pitch) :
        Front(glm::dvec3(0.f, 0.f, -1.f)), Position(glm::dvec3(posX, posY, posZ)), WorldUp(glm::dvec3(upX, upY, upZ)),
        Yaw(yaw), Pitch(pitch), MovementSpeed(CAMERASPEED), MouseSensitivity(SENSITIVITY), Fov(FOV)
    {
        __updateCameraVectors();
    }

    glm::mat4 getViewMatrix() const {
        return glm::lookAt(Position, Position + Front, Up);
    }

    glm::dmat4 getViewMatrixDouble() const
    {
        return glm::lookAt(Position, Position + Front, Up);
    }

    glm::mat4 getProjectionMatrix(uint32_t vWidth, uint32_t vHeight) const {
        return glm::perspective(glm::radians(Fov), static_cast<double>(vWidth) / vHeight, ZNear, ZFar);
    }

    glm::mat4 getProjectViewMatrix(uint32_t vWidth, uint32_t vHeight) const {
        return getProjectionMatrix(vWidth, vHeight) * getViewMatrix();
    }

    void setSensitivity(double f)
    {
        MouseSensitivity = f;
    }
    void setMoveSpeed(double f)
    {
        MovementSpeed = f;
    }

    void processKeyboard(CameraMoveDirection vDirection, double vDeltaTime) {
        double Velocity = MovementSpeed * vDeltaTime;
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

    void processMouseMovement(double vXoffset, double vYoffset, GLboolean constrainPitch = true)
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

    void processMouseScroll(double vYoffset)
    {
        Fov -= (double)vYoffset;
        if (Fov < 1.f) Fov = 1.f;
        if (Fov > 55.f) Fov = 55.f;
    }

    std::tuple<glm::vec3, glm::vec3> getRay(const glm::vec2& vWindowPos, const glm::uvec2& vResolution) const
    {
        const glm::mat4 view = getViewMatrix();
        const glm::mat4 projection = getProjectionMatrix(vResolution.x, vResolution.y);
        const glm::vec4 viewport = glm::vec4(0, 0, vResolution.x, vResolution.y);
        const glm::vec3 p1 = glm::unProject(glm::vec3(vWindowPos, 0), view, projection, viewport);
        const glm::vec3 p2 = glm::unProject(glm::vec3(vWindowPos, 1), view, projection, viewport);

        return std::make_tuple(p1, glm::normalize(p2 - p1));
    }

private:
    void __updateCameraVectors() {
        glm::dvec3 front;
        front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        front.y = sin(glm::radians(Pitch));
        front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        Front = glm::normalize(front);
        Right = glm::normalize(glm::cross(Front, WorldUp));
        Up = glm::normalize(glm::cross(Right, Front));
    }
};
#endif