#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <iostream>

#include "utils.hpp"
#include "camera.hpp"

namespace tinyglrenderer {

struct alignas(16) LightBlock {
    glm::mat4 viewProjMatrix;
    glm::vec4 colorIntensity;
    glm::vec4 vectorType;  // .xyz vector(direction or position) .w type(0: directional or 1: point)
    glm::vec4 uvOffsetScale;
    glm::vec4 sceneBounds; // .x height .y width .zw reserved
};

struct alignas(16) MainLightBlock { // supplementary light block for main light source generating CSM
    glm::mat4 viewProjMatrices[4];
    glm::vec4 splits; // x split[0], y split[1], z split[2], w split[3]
    glm::vec4 radiuses; // the radius of each level of cascaded shadow map, used for dynamic bias
};

class Light {
   public:
    Light() = default;
    Light(const glm::vec3& color, float intensity) {
        m_lightBlock.colorIntensity = glm::vec4(color, intensity);
    }
    virtual ~Light() = default;

    bool isVisible() const { return m_visible; }
    glm::vec3 getColor() const { return glm::vec3(m_lightBlock.colorIntensity); }
    float getIntensity() const { return m_lightBlock.colorIntensity.w; }
    const glm::mat4& getViewProjMatrix() const { return m_lightBlock.viewProjMatrix; }
    const glm::mat4& getViewProjMatrix(int layer) const { 
        if (m_mainLightBlock == nullptr) { throw std::runtime_error("Light::getViewProjMatrix: Main light block is not set");}
        return m_mainLightBlock->viewProjMatrices[layer];
    }
    const LightBlock& getLightBlock() const { return m_lightBlock; }
    const MainLightBlock& getMainLightBlock() const { 
        if (m_mainLightBlock == nullptr) { throw std::runtime_error("Light::getMainLightBlock: Main light block is not set");}
        return *m_mainLightBlock; 
    }
    void setMainLight(bool main) { if (main) { m_mainLightBlock = std::make_unique<MainLightBlock>(); } }
    void setColor(const glm::vec3& color) {
        m_lightBlock.colorIntensity.x = color.x;
        m_lightBlock.colorIntensity.y = color.y;
        m_lightBlock.colorIntensity.z = color.z;
    }
    void setIntensity(float intensity) {
        m_lightBlock.colorIntensity.w = intensity;
    }
    void setUVOffsetScale(const glm::vec2& offset, const glm::vec2& scale) {
        m_lightBlock.uvOffsetScale = glm::vec4(offset, scale);
    }
    void setVisible(bool visible) { m_visible = visible; }
    virtual void setLightSpaceMatrix(const std::pair<glm::vec3, glm::vec3>& bounds, const std::shared_ptr<Camera>& camera) = 0;

   protected:
    LightBlock m_lightBlock;
    std::unique_ptr<MainLightBlock> m_mainLightBlock;
    bool m_visible = true; // on/off
};

class DirectionalLight : public Light {
   public:
    DirectionalLight() = default;
    DirectionalLight(const glm::vec3& color, float intensity, const glm::vec3& direction) : Light(color, intensity) {
        m_lightBlock.vectorType = glm::vec4(glm::normalize(direction), 0.0f);
    }
    ~DirectionalLight() = default;

    glm::vec3 getDirection() const { return glm::vec3(m_lightBlock.vectorType); }
    void setDirection(const glm::vec3& direction) {
        m_lightBlock.vectorType = glm::vec4(glm::normalize(direction), 0.0f);
    }
    inline void setLightSpaceMatrix(const std::pair<glm::vec3, glm::vec3>& bounds, const std::shared_ptr<Camera>& camera) override;
};

inline void DirectionalLight::setLightSpaceMatrix(const std::pair<glm::vec3, glm::vec3>& bounds, const std::shared_ptr<Camera>& camera) {
    // 0. Get the camera info to construct frustum
    glm::vec3 direction = this->getDirection();
    float fovy   = camera->getFov();
    float aspect = camera->getAspect();
    float near  = camera->getNear();
    float far = camera->getFar();

    // 1. Calculate splits for CSM
    bool isMainLight = (m_mainLightBlock != nullptr);
    size_t cascadeCount = isMainLight ? 4 : 1;
    std::vector<float> splits(cascadeCount + 1);
    splits.front() = near;
    splits.back()  = far;
    constexpr float lambda = 0.75f;
    for (size_t i = 1; i < cascadeCount; ++i) {
        float f = static_cast<float>(i) / static_cast<float>(cascadeCount);
        float logSplit    = splits.front() * std::pow(splits.back() / splits.front(), f);
        float linearSplit = splits.front() + (splits.back() - splits.front()) * f;
        splits[i] = lambda * logSplit + (1.0f - lambda) * linearSplit;
    }

    // 2. Get the scene corners in world space
    auto& [xyz1, xyz2]  = bounds;
    glm::vec3 sceneCornersWS[8] = {
        {xyz1.x, xyz1.y, xyz1.z}, {xyz2.x, xyz1.y, xyz1.z},
        {xyz1.x, xyz2.y, xyz1.z}, {xyz2.x, xyz2.y, xyz1.z},
        {xyz1.x, xyz1.y, xyz2.z}, {xyz2.x, xyz1.y, xyz2.z},
        {xyz1.x, xyz2.y, xyz2.z}, {xyz2.x, xyz2.y, xyz2.z},
    };

    // 3. Use scene bounds to calculate light space matrix for non-main light
    if (!isMainLight) {
        glm::vec3 sceneCenter = (xyz1 + xyz2) / 2.0f;
        glm::vec3 lightPos    = sceneCenter - direction * glm::length(xyz2 - xyz1) * 0.75f;
        glm::vec3 lightUp     = {0.0f, 1.0f, 0.0f};
        if (std::abs(glm::dot(direction, lightUp)) > 0.99f) {
            lightUp = {0.0f, 0.0f, 1.0f};
        }
        glm::mat4 lightViewMatrix = glm::lookAt(lightPos, sceneCenter, lightUp);

        glm::vec3 nxyz1 = glm::vec3(FLT_MAX), nxyz2 = glm::vec3(-FLT_MAX);
        for (int i = 0; i < 8; ++i) {
            glm::vec3 sceneCornerLVS = glm::vec3(lightViewMatrix * glm::vec4(sceneCornersWS[i], 1.0f));
            nxyz1 = glm::min(nxyz1, sceneCornerLVS);
            nxyz2 = glm::max(nxyz2, sceneCornerLVS);
        }
        glm::mat4 lightProjMatrix = glm::ortho(
            nxyz1.x, nxyz2.x,
            nxyz1.y, nxyz2.y,
            -nxyz2.z,  // near
            -nxyz1.z   // far
        );
        m_lightBlock.viewProjMatrix = lightProjMatrix * lightViewMatrix;
        m_lightBlock.sceneBounds = glm::vec4(nxyz2.x - nxyz1.x, nxyz2.y - nxyz1.y, 0.0f, 0.0f);
        return ;
    }

    // 4. Use camera frustum to calculate light space matrix for main light
    // https://lxjk.github.io/2017/04/15/Calculate-Minimal-Bounding-Sphere-of-Frustum.html
    float k = std::tan(fovy * 0.5f) * std::sqrt(1 + aspect * aspect);
    float k2 = k * k;
    float k4 = k2 * k2;
    for (size_t i = 0; i < cascadeCount; ++i) {
        // 3.1 Construct a sphere to cover frustum slice in view space
        float n = splits[i]; // near plane of current frustum slice
        float f = splits[i + 1]; // far plane of current frustum slice
        glm::vec3 center;
        float radius;
        if (k2 >= (f - n) / (f + n)) {
            center = glm::vec3(0, 0, -f);
            radius = f * k;
        } else {
            radius  = 0.5 * sqrt((f - n) * (f - n) + 2.0f * (f * f + n * n) * k2 + (f + n) * (f + n) * k4);
            center = glm::vec3(0, 0, -0.5f * (f + n) * (1.0f + k2));
        }
        
        // 3.2 Calculate light space matrix
        glm::vec3 lightPos = center - direction * radius * 2.f;
        glm::vec3 lightUp  = {0.0f, 1.0f, 0.0f};
        if (std::abs(glm::dot(direction, lightUp)) > 0.99f) { lightUp = {0.0f, 0.0f, 1.0f}; }
        glm::mat4 lightViewMatrix = glm::lookAt(lightPos, center, lightUp);

        // 3.3 Calculate light space projection matrix
        glm::mat4 lightProjMatrix = glm::ortho(-radius, radius, -radius, radius, -radius * 4.0f, radius * 4.0f);

        // 3.4 Update main light block
        if (m_mainLightBlock) {
            m_mainLightBlock->viewProjMatrices[i] = lightProjMatrix * lightViewMatrix;
            m_mainLightBlock->splits[i] = -splits[i + 1]; // only keep z far and flip the sign
            m_mainLightBlock->radiuses[i] = radius;
        }
        if (i == 0) {
            m_lightBlock.viewProjMatrix = lightProjMatrix * lightViewMatrix;
        }
    }

    return;
}

};  // namespace tinyglrenderer