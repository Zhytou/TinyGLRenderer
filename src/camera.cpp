#include "camera.hpp"

namespace tinyglrenderer {

void Camera::rotate(float yaw, float pitch, float sensitivity) {
    float angleX = glm::radians(-yaw * sensitivity);
    float angleY = glm::radians(-pitch * sensitivity);

    glm::vec3 front = glm::normalize(m_target - m_eye);
    glm::vec3 right = glm::normalize(glm::cross(front, m_up));

    glm::mat4 rotationX = glm::rotate(glm::mat4(1.0f), angleX, glm::vec3(0.0f, 1.0f, 0.0f));
    front               = glm::vec3(rotationX * glm::vec4(front, 0.0f));
    float currentPitch  = glm::degrees(glm::asin(front.y));
    if (currentPitch + glm::degrees(angleY) < 85.0f && currentPitch + glm::degrees(angleY) > -85.0f) {
        glm::mat4 rotationY = glm::rotate(glm::mat4(1.0f), angleY, right);
        front               = glm::vec3(rotationY * glm::vec4(front, 0.0f));
    }

    float distance = glm::distance(m_eye, m_target);
    m_target       = m_eye + glm::normalize(front) * distance;

    update();
}

void Camera::move(CameraMovement direction, float deltaTime) {
    float velocity = getSpeed() * deltaTime;

    glm::vec3 front = glm::normalize(m_target - m_eye);
    glm::vec3 right = glm::normalize(glm::cross(front, m_up));
    glm::vec3 up    = glm::normalize(glm::cross(right, front));

    glm::vec3 offset(0.0f);
    switch (direction) {
        case CameraMovement::FORWARD: offset = front * velocity; break;
        case CameraMovement::BACKWARD: offset = -front * velocity; break;
        case CameraMovement::LEFT: offset = -right * velocity; break;
        case CameraMovement::RIGHT: offset = right * velocity; break;
        case CameraMovement::UPWARD: offset = up * velocity; break;
        case CameraMovement::DOWNWARD: offset = -up * velocity; break;
    }
    m_eye += offset;
    m_target += offset;
    update();
}

void Camera::zoom(float offset, float sensitivity) {
    auto direction  = offset > 0.0f ? CameraMovement::FORWARD : CameraMovement::BACKWARD;
    float deltaTime = fabs(offset) * sensitivity;
    move(direction, deltaTime);
}

}  // namespace tinyglrenderer