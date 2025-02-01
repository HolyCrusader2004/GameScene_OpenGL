#include "Camera.hpp"

namespace gps {

    float yaw, pitch;
    // Camera constructor
    Camera::Camera(glm::vec3 cameraPosition, glm::vec3 cameraTarget, glm::vec3 cameraUp) {
        this->cameraPosition = cameraPosition;
        this->cameraTarget = cameraTarget;
        this->cameraUpDirection = cameraUp;


        cameraFrontDirection = glm::normalize(cameraTarget - cameraPosition);
        cameraRightDirection = glm::normalize(glm::cross(cameraFrontDirection, cameraUpDirection));
        yaw = glm::degrees(atan2(cameraFrontDirection.z, cameraFrontDirection.x)); 
        pitch = glm::degrees(asin(cameraFrontDirection.y)); 
        this->cameraTarget = cameraPosition + cameraFrontDirection;
    }

    glm::mat4 Camera::getViewMatrix() {
        return glm::lookAt(cameraPosition, cameraTarget, cameraUpDirection);
    }

    void Camera::move(MOVE_DIRECTION moveDir, float delta) {
        glm::vec3 movementOffset(0.0f);

        if (moveDir == MOVE_FORWARD) {
            movementOffset += delta * cameraFrontDirection;
        }
        else if (moveDir == MOVE_BACKWARD) {
            movementOffset -= delta * cameraFrontDirection;
        }
        else if (moveDir == MOVE_RIGHT) {
            movementOffset += delta * cameraRightDirection;
        }
        else if (moveDir == MOVE_LEFT) {
            movementOffset -= delta * cameraRightDirection;
        }

        cameraPosition += movementOffset;
        cameraTarget = cameraPosition + cameraFrontDirection;
    }

    void Camera::rotate(float deltaPitch, float deltaYaw) {
        pitch += deltaPitch;
        yaw += deltaYaw;

        pitch = glm::clamp(pitch, -89.0f, 89.0f);

        float cosPitch = cos(glm::radians(pitch));
        glm::vec3 newFront;
        newFront.x = cos(glm::radians(yaw)) * cosPitch;
        newFront.y = sin(glm::radians(pitch));
        newFront.z = sin(glm::radians(yaw)) * cosPitch;

        cameraFrontDirection = glm::normalize(newFront);
        cameraTarget = cameraPosition + cameraFrontDirection;
        cameraRightDirection = glm::normalize(glm::cross(cameraFrontDirection, cameraUpDirection));
    }

    glm::vec3 gps::Camera::getCameraPosition() const {
        return cameraPosition;
    }

    float gps::Camera::getYaw() const {
        return yaw;
    }

    glm::vec3 gps::Camera::getCameraDirection() const {
       return glm::normalize(cameraTarget - cameraPosition);
    }
    void Camera::setPosition(glm::vec3 newPos) {
        cameraPosition = newPos;
    }
 
    void Camera::setDirection(const glm::vec3& newDirection) {
        cameraFrontDirection = glm::normalize(newDirection);

        cameraTarget = cameraPosition + cameraFrontDirection;
        cameraRightDirection = glm::normalize(glm::cross(cameraFrontDirection, cameraUpDirection));
    }

}