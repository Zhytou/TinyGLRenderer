#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace tinyglrenderer {

struct alignas(16) CameraBlock {
    glm::mat4 viewMatrix;
    glm::mat4 projMatrix;
    glm::mat4 invViewMatrix;
    glm::mat4 invProjMatrix;
    glm::vec3 position;
    float type; // 0: perspective, 1: orthographic
    float fov;
    float near;
    float far;
    float aspect;
};

enum class CameraMovement {
    FORWARD,  // POSITIVE_Z
    BACKWARD, // NEGATIVE_Z
    LEFT,     // NEGATIVE_X
    RIGHT,    // POSITIVE_X
    UPWARD,   // POSITIVE_Y
    DOWNWARD, // NEGATIVE_Y
};

class Camera {
   public:
    Camera()          = default;
    virtual ~Camera() = default;

    // Rotate camera to new direction
    // @param yaw Yaw angle in degrees(Left to Right, -180 to 180, oriented around Y axis)
    // @param pitch Pitch angle in degrees(Up to Down, -90 to 90, oriented around X axis)
    // @param sensitivity Sensitivity of the camera rotation
    void rotate(float yaw, float pitch, float sensitivity = 0.1f);
    // Move camera to new position
    // @param direction CameraMovement direction
    // @param deltaTime Time delta in seconds
    void move(CameraMovement direction, float deltaTime);
    // Zoom camera to new position
    // @param offset Zoom offset
    // @param sensitivity Sensitivity of the camera zoom
    void zoom(float offset, float sensitivity = 0.1f);
    // Reset camera to original position
    void reset() {
        m_eye    = m_eyeOriginal;
        m_target = m_targetOriginal;
        update();
    }
    virtual void update() = 0;

    float getSpeed() const { return m_speed; }
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    float getFov() const { return m_fov; }
    float getNear() const { return m_near; }
    float getFar() const { return m_far; }
    float getAspect() const { return m_aspect; }
    float getDistance(const glm::vec3& position) const { return glm::distance(m_eye, position); }
    const glm::vec3& getEye() const { return m_eye; }
    const glm::vec3& getTarget() const { return m_target; }
    const glm::mat4& getViewMatrix() const { return m_cameraBlock.viewMatrix; }
    const glm::mat4& getInvViewMatrix() const { return m_cameraBlock.invViewMatrix; }
    const glm::mat4& getProjMatrix() const { return m_cameraBlock.projMatrix; }
    const glm::mat4& getInvProjMatrix() const { return m_cameraBlock.invProjMatrix; }
    const CameraBlock& getCameraBlock() const { return m_cameraBlock; }

    void setSpeed(float speed) {
        m_speed = speed;
    }
    void setEye(glm::vec3 eye) {
        if (std::isnan(m_eyeOriginal.x) || std::isnan(m_eyeOriginal.y) || std::isnan(m_eyeOriginal.z)) {
            m_eyeOriginal = eye;
        }
        m_eye = eye;
        update();
    }
    void setTarget(glm::vec3 target) {
        if (std::isnan(m_targetOriginal.x) || std::isnan(m_targetOriginal.y) || std::isnan(m_targetOriginal.z)) {
            m_targetOriginal = target;
        }
        m_target = target;
        update();
    }
    void setUp(glm::vec3 up) {
        m_up = up;
        update();
    }
    void setFov(float fov) {
        m_fov = fov;
        update();
    }
    void setNear(float near) {
        m_near = near;
        update();
    }
    void setFar(float far) {
        m_far = far;
        update();
    }
    void setAspect(int width, int height) {
        m_width  = width;
        m_height = height;
        m_aspect = static_cast<float>(width) / static_cast<float>(height);
        update();
    }

   protected:
    CameraBlock m_cameraBlock;
    glm::vec3 m_eye;
    glm::vec3 m_target;
    glm::vec3 m_up;

    glm::vec3 m_eyeOriginal    = glm::vec3(std::nanf(""));
    glm::vec3 m_targetOriginal = glm::vec3(std::nanf(""));

    float m_fov;
    float m_near;
    float m_far;
    float m_aspect;
    int m_width;
    int m_height;
    float m_speed;
};

class PerspectiveCamera : public Camera {
   public:
    PerspectiveCamera()  = default;
    ~PerspectiveCamera() = default;

   protected:
    void update() override {
        m_cameraBlock.viewMatrix        = glm::lookAt(m_eye, m_target, m_up);
        m_cameraBlock.projMatrix        = glm::perspective(glm::radians(m_fov), m_aspect, m_near, m_far);
        m_cameraBlock.invViewMatrix     = glm::inverse(m_cameraBlock.viewMatrix);
        m_cameraBlock.invProjMatrix     = glm::inverse(m_cameraBlock.projMatrix);
        m_cameraBlock.position          = m_eye;
        m_cameraBlock.type              = 0.0f;
        m_cameraBlock.fov               = m_fov;
        m_cameraBlock.near              = m_near;
        m_cameraBlock.far               = m_far;
        m_cameraBlock.aspect            = m_aspect;
    }
};

class OrthographicCamera : public Camera {
   public:
    OrthographicCamera()  = default;
    ~OrthographicCamera() = default;

   protected:
    void update() override {
        m_cameraBlock.viewMatrix        = glm::lookAt(m_eye, m_target, m_up);
        m_cameraBlock.projMatrix        = glm::ortho(-m_width / 2.0f, m_width / 2.0f, -m_height / 2.0f, m_height / 2.0f, m_near, m_far);
        m_cameraBlock.invViewMatrix     = glm::inverse(m_cameraBlock.viewMatrix);
        m_cameraBlock.invProjMatrix     = glm::inverse(m_cameraBlock.projMatrix);
        m_cameraBlock.type              = 1.0f;
        m_cameraBlock.position          = m_eye;
        m_cameraBlock.fov               = m_fov;
        m_cameraBlock.near              = m_near;
        m_cameraBlock.far               = m_far;
        m_cameraBlock.aspect            = m_aspect;
    }
};

} // namespace tinyglrenderer