#pragma once

#include <memory>
#include <string>
#include <vector>

#include "camera.hpp"
#include "light.hpp"
#include "model.hpp"
#include "renderitem.hpp"
#include "texture.hpp"
#include "resourcemanager.hpp"

namespace tinyglrenderer {

class Scene {
   public:
    Scene()                        = default;
    Scene(const Scene&)            = delete;
    Scene& operator=(const Scene&) = delete;
    ~Scene() { destroy(); }

    const std::shared_ptr<Texture>& getSkyboxCubeMap() const { return m_skyboxCubemap; }
    const std::shared_ptr<Texture>& getSkyboxEquirect() const { return m_skyboxEquirect; }
    const std::shared_ptr<Camera>& getCamera() const { return m_camera; }
    std::shared_ptr<Light> getMainLight() const { return m_lights.empty() ? nullptr : m_lights[0]; }
    const std::vector<std::shared_ptr<Light>>& getLights() const { return m_lights; }
    size_t getMaxLightCount() const { return m_lights.size(); }
    size_t getVisibleLightCount() const;
    const std::vector<std::shared_ptr<Model>>& getModels() const { return m_models; }
    size_t getMaxModelCount() const { return m_models.size(); }
    size_t getVisibleModelCount() const;
    const std::pair<glm::vec3, glm::vec3>& getBoundingBox() const { return m_bounds; }
    void getModelBlocks(std::vector<ModelBlock>& blocks) const;
    void getLightBlocks(std::vector<LightBlock>& blocks) const;
    void getRenderQueue(std::vector<RenderItem>& queue, bool opaque, bool reset = true) const;

    void initialize(const std::string& json, ResourceManager& manager);
    void destroy();

   private:
    std::shared_ptr<Texture> m_skyboxCubemap  = nullptr;
    std::shared_ptr<Texture> m_skyboxEquirect = nullptr;
    std::shared_ptr<Camera> m_camera          = nullptr;
    std::vector<std::shared_ptr<Light>> m_lights;
    std::vector<std::shared_ptr<Model>> m_models;
    std::pair<glm::vec3, glm::vec3> m_bounds = {glm::vec3(0.0f), glm::vec3(0.0f)};
};

}  // namespace tinyglrenderer
