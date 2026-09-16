#include "model.hpp"

#include <format>
#include <iostream>
#include <stdexcept>
#include <unordered_map>
#include <vector>

#include "utils.hpp"

namespace tinyglrenderer {

namespace fs = std::filesystem;

Model::Model() {}

Model::Model(const std::string& name, const std::shared_ptr<Mesh>& mesh, const std::vector<std::shared_ptr<Material>>& materials, const std::shared_ptr<Material>& defaultMaterial) {
    m_name      = name;
    m_mesh      = std::move(mesh);
    m_materials = materials;
    m_material  = defaultMaterial;
}

Model::~Model() {
    m_mesh.reset();
    m_materials.clear();
    m_material.reset();
}

Model::Model(Model&& other) : m_mesh(std::move(other.m_mesh)),
                              m_materials(std::move(other.m_materials)),
                              m_modelBlock(other.m_modelBlock) {
}

Model& Model::operator=(Model&& other) {
    if (this != &other) {
        m_mesh       = std::move(other.m_mesh);
        m_materials  = std::move(other.m_materials);
        m_modelBlock = other.m_modelBlock;
    }
    return *this;
}

void Model::getRenderQueue(std::vector<RenderItem>& queue, bool opaque) const {
    if (!m_visible) { return; }

    for (auto& sm : m_mesh->getSubMeshes()) {
        auto material = sm.matid != -1 ? m_materials[sm.matid] : m_material;
        if (material == nullptr) { throw std::runtime_error(std::format("Model::getRenderQueue: Invalid material for {} submesh {}!", m_name, sm.matid)); }
        if (material->isOpaque() != opaque) { continue; }
        queue.emplace_back(
            RenderItem{
                .mesh     = m_mesh,
                .material = material,
                .ioffset  = sm.offset,
                .length   = sm.length,
            }
        );
    }
}

void Model::setTransform(const glm::vec3& translate, const glm::vec3& rotate, const glm::vec3& scale) {
    glm::mat4 transform = glm::mat4(1.0f);

    transform = glm::translate(transform, translate);
    transform = glm::rotate(transform, glm::radians(rotate.x), glm::vec3(1.0f, 0.0f, 0.0f));
    transform = glm::rotate(transform, glm::radians(rotate.y), glm::vec3(0.0f, 1.0f, 0.0f));
    transform = glm::rotate(transform, glm::radians(rotate.z), glm::vec3(0.0f, 0.0f, 1.0f));
    transform = glm::scale(transform, scale);

    // update transforms
    m_transforms["translate"] = translate;
    m_transforms["rotate"]    = rotate;
    m_transforms["scale"]     = scale;

    // update transform matrix
    m_modelBlock.transformMatrix = transform;
    m_modelBlock.normalMatrix    = glm::mat4(glm::transpose(glm::inverse(glm::mat3(transform))));

    // update bounding box
    auto [xyz1, xyz2] = m_mesh->getBoundingBox();
    xyz1              = glm::vec3(transform * glm::vec4(xyz1, 1.f));
    xyz2              = glm::vec3(transform * glm::vec4(xyz2, 1.f));
    m_bounds          = {xyz1, xyz2};
}

} // namespace tinyglrenderer
