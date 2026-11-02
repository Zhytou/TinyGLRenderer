#include "resourcemanager.hpp"

#include <vector>
#include <stdexcept>
#include <format>

#include "utils.hpp"

namespace tinyglrenderer {

namespace fs = std::filesystem;

std::unordered_map<std::string, GLsizei> ResourceManager::m_counts;
std::unordered_map<std::string, std::shared_ptr<VertexLayout>> ResourceManager::m_layouts;
std::unordered_map<std::string, std::unique_ptr<VertexBuffer>> ResourceManager::m_buffers;
std::array<glm::mat4, 6> ResourceManager::m_matrixs;

void ResourceManager::initialize() {
    // 1. Define vertex layouts
    m_layouts["mesh"] = std::make_shared<VertexLayout>();
    m_layouts["mesh"]->initialize({
        VertexAttribute{
            .location   = 0,
            .size       = 3,
            .type       = GL_FLOAT,
            .offset     = offsetof(Vertex, position),
            .normalized = GL_FALSE,
            .stride     = sizeof(Vertex),
            .slot       = 0,
        },
        VertexAttribute{
            .location   = 1,
            .size       = 3,
            .type       = GL_FLOAT,
            .offset     = offsetof(Vertex, normal),
            .normalized = GL_TRUE,
            .stride     = sizeof(Vertex),
            .slot       = 0,
        },
        VertexAttribute{
            .location   = 2,
            .size       = 3,
            .type       = GL_FLOAT,
            .offset     = offsetof(Vertex, tangent),
            .normalized = GL_TRUE,
            .stride     = sizeof(Vertex),
            .slot       = 0,
        },
        VertexAttribute{
            .location   = 3,
            .size       = 2,
            .type       = GL_FLOAT,
            .offset     = offsetof(Vertex, texcoord),
            .normalized = GL_FALSE,
            .stride     = sizeof(Vertex),
            .slot       = 0,
        },
    });

    m_layouts["quad"] = std::make_shared<VertexLayout>();
    m_layouts["quad"]->initialize({
        VertexAttribute{
            .location   = 0,
            .size       = 2,
            .type       = GL_FLOAT,
            .offset     = 0,
            .normalized = GL_FALSE,
            .stride     = sizeof(float) * 4,
            .slot       = 0,
        },
        VertexAttribute{
            .location   = 1,
            .size       = 2,
            .type       = GL_FLOAT,
            .offset     = sizeof(float) * 2,
            .normalized = GL_FALSE,
            .stride     = sizeof(float) * 4,
            .slot       = 0,
        },
    });

    m_layouts["cube"] = std::make_shared<VertexLayout>();
    m_layouts["cube"]->initialize({
        VertexAttribute{
            .location   = 0,
            .size       = 3,
            .type       = GL_FLOAT,
            .offset     = 0,
            .normalized = GL_FALSE,
            .stride     = sizeof(float) * 3,
            .slot       = 0,
        },
    });

    // 2. Create static vertex buffers for quad and cube
    float quad[][4] = {
        // Position(2f) and TexCoords(2f)
        {-1.0f, 1.0f, 0.0f, 1.0f},
        {-1.0f, -1.0f, 0.0f, 0.0f},
        {1.0f, -1.0f, 1.0f, 0.0f},
        {-1.0f, 1.0f, 0.0f, 1.0f},
        {1.0f, -1.0f, 1.0f, 0.0f},
        {1.0f, 1.0f, 1.0f, 1.0f},
    };
    m_buffers["quad"] = std::make_unique<VertexBuffer>(sizeof(quad), quad);

    float cube[][18] = {
        // +X Position(3f) * 6
        {1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f},
        // -X
        {-1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, -1.0f},
        // +Y
        {-1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f},
        // -Y
        {-1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f},
        // +Z
        {-1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f, 1.0f},
        // -Z
        {-1.0f, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f},
    };
    m_buffers["cube"] = std::make_unique<VertexBuffer>(sizeof(cube), cube);

    // 3. Calculate vertex counts for quad and cube
    m_counts["quad"] = sizeof(quad) / (sizeof(float) * 4);
    m_counts["cube"] = sizeof(cube) / (sizeof(float) * 3);

    // 4. Attach vertex layouts to buffers for quad and cube
    m_layouts["quad"]->attach(0, m_buffers["quad"], 0, sizeof(float) * 4);
    m_layouts["cube"]->attach(0, m_buffers["cube"], 0, sizeof(float) * 3);

    // 5. Initialize view projection matrices
    glm::mat4 projMatrix    = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    glm::mat4 viewMatrixs[] = {glm::lookAt(glm::vec3(0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)), glm::lookAt(glm::vec3(0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)), glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)), glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)), glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)), glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f))};
    for (GLenum face = GL_TEXTURE_CUBE_MAP_POSITIVE_X; face <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z; face++) {
        int index        = face - GL_TEXTURE_CUBE_MAP_POSITIVE_X;
        m_matrixs[index] = projMatrix * viewMatrixs[index];
    }
}

void ResourceManager::destroy() {
    m_layouts.clear();
    m_buffers.clear();
    m_materials.clear();
    m_meshes.clear();
    m_textures.clear();
    m_images.clear();
}

const GLsizei& ResourceManager::getCount(const std::string& name) {
    if (!m_counts.count(name)) { throw std::runtime_error("ResourceManager::getCount: Vertex count for " + name + " not found in ResourceManager"); }
    return m_counts.at(name);
}

const std::shared_ptr<VertexLayout>& ResourceManager::getLayout(const std::string& name) {
    if (!m_layouts.count(name)) { throw std::runtime_error("ResourceManager::getLayout: VertexLayout " + name + " not found in ResourceManager"); }
    return m_layouts.at(name);
}

const std::unique_ptr<VertexBuffer>& ResourceManager::getBuffer(const std::string& name) {
    if (!m_buffers.count(name)) { throw std::runtime_error("ResourceManager::getBuffer: VertexBuffer " + name + " not found in ResourceManager"); }
    return m_buffers.at(name);
}

const glm::mat4& ResourceManager::getCaptureMatrix(GLint index) {
    if (index < 0 || index >= 6) { throw std::runtime_error(std::format("ResourceManager::getCaptureMatrix: CaptureMatrix {} not found in ResourceManager", index)); }
    return m_matrixs[index];
}

std::shared_ptr<Mesh> ResourceManager::getMesh(const std::string& name) const {
    if (m_meshes.count(name) == 0 || m_meshes.at(name).expired()) {
        throw std::runtime_error(std::format("ResourceManager::getMesh: mesh {} does not exist!", name));
    }
    return m_meshes.at(name).lock();
}

std::shared_ptr<Texture> ResourceManager::getTexture(const std::string& name) const {
    if (m_textures.count(name) == 0 || m_textures.at(name).expired()) {
        throw std::runtime_error(std::format("ResourceManager::getShader: texture {} does not exist!", name));
    }
    return m_textures.at(name).lock();
}

std::shared_ptr<Shader> ResourceManager::getShader(const std::string& name) const {
    if (m_shaders.count(name) == 0 || m_shaders.at(name).expired()) {
        throw std::runtime_error(std::format("ResourceManager::getShader: shader {} does not exist!", name));
    }
    return m_shaders.at(name).lock();
}

void ResourceManager::getAllMeshNames(std::vector<std::string>& names) const {
    names.clear();
    for (auto& [name, mesh] : m_meshes) {
        if (mesh.expired()) continue;
        names.push_back(name);
    }
}

void ResourceManager::getAllTextureNames(std::vector<std::string>& names) const {
    names.clear();
    for (auto& [name, texture] : m_textures) {
        if (texture.expired()) continue;
        if (name.starts_with("default_")) continue; // skip default texture generated from glm::vec4 value
        names.push_back(name);
    }
}
void ResourceManager::getAllShaderNames(std::vector<std::string>& names) const {
    names.clear();
    for (auto& [name, shader] : m_shaders) {
        if (shader.expired()) continue;
        names.push_back(name);
    }
}

std::shared_ptr<Model> ResourceManager::loadModel(const std::string& modelName, const fs::path& objPath, const fs::path& mtlDir) {
    // 1. Load obj model with tinyobj loader
    tinyobj::attrib_t attributes;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string err;
    if (!tinyobj::LoadObj(&attributes, &shapes, &materials, &err, objPath.c_str(), mtlDir.c_str(), true)) {
        throw std::runtime_error("ResourceManager::loadModel: " + err);
    }

    // 2. Create mesh from tinyobj shapes
    std::cout << "Loading mesh [" << objPath << "]\n";
    std::shared_ptr<Mesh> mesh = loadMesh(objPath.stem().string(), objPath, attributes, shapes, materials.size());

    // 3. Convert tinyobj material into self-defined material
    std::cout << "Loading materials [";
    std::vector<std::shared_ptr<Material>> nmaterials;
    for (auto& material : materials) {
        std::cout << material.name << ", ";
        nmaterials.push_back(loadMaterial(material.name, mtlDir, material));
    }
    std::cout << "]\n";

    return make_shared<Model>(modelName, mesh, nmaterials);
}

std::shared_ptr<Mesh> ResourceManager::loadMesh(const std::string& meshName, const fs::path& meshPath, const tinyobj::attrib_t& attributes, const std::vector<tinyobj::shape_t>& shapes, size_t num) {
    if (m_meshes.count(meshName) && !m_meshes[meshName].expired()) {
        return m_meshes[meshName].lock();
    }

    std::shared_ptr<Mesh> mesh = std::make_shared<Mesh>(meshPath, attributes, shapes, num);
    m_meshes[meshName] = mesh;
    
    return mesh;
}

std::shared_ptr<Material> ResourceManager::loadMaterial(const std::string& matName, const fs::path& matDir, const tinyobj::material_t& material) {
    if (m_materials.count(matName) && !m_materials[matName].expired()) {
        return m_materials[matName].lock();
    }

    glm::vec4 albedo = glm::vec4(material.diffuse[0], material.diffuse[1], material.diffuse[2], 1.0f);
    glm::vec4 normal = glm::vec4(0.5f, 0.5f, 1.0f, 1.f);
    glm::vec4 mrao = glm::vec4(material.metallic, material.roughness, 0.f, 1.f);

    std::unordered_map<std::string, std::shared_ptr<Texture>> textures = {
        // TODO: fix mip level(when miplevel is more than 1, the render result is wrong, blocking artifacts appear)
        {"albedo", load2DTexture(std::format("{}_albedo", matName), matDir / material.diffuse_texname, albedo, GL_RGBA32F)},   
        {"normal", load2DTexture(std::format("{}_normal", matName), matDir / material.normal_texname, normal, GL_RGBA32F)}, 
        {"mrao", load2DTexture(std::format("{}_mrao", matName), {matDir / material.metallic_texname, matDir / material.roughness_texname, matDir / material.ambient_texname}, mrao, GL_RGBA32F, 1, 1)}
    };
    auto nmaterial = std::make_shared<Material>(matName, material.dissolve, textures);
    m_materials[matName] = nmaterial;

    return nmaterial;
}

std::shared_ptr<Texture> ResourceManager::load2DTexture(const std::string& texName, const fs::path& texPath, const glm::vec4& defaultValue, GLenum internalFormat, GLsizei mipLevels, int desiredChannels, bool verticalFlip) {
    std::string texAlias = std::format("default_2d_tex_color({:.3f}, {:.3f}, {:.3f}, {:.3f})", defaultValue.x, defaultValue.y, defaultValue.z, defaultValue.w);
    if (m_textures.count(texName) && !m_textures[texName].expired()) {
        return m_textures[texName].lock();
    }
    
    std::shared_ptr<Texture> texture;
    if (fs::is_regular_file(texPath)) {
        std::cout << "Loading texture(GL_TEXTURE_2D) from file [" << texPath << "]\n";
        std::shared_ptr<Image> image = loadImage(texName, texPath, desiredChannels, verticalFlip);
        texture = std::make_shared<Texture>(image->getWidth(), image->getHeight(), GL_TEXTURE_2D, internalFormat, mipLevels);
        texture->upload(image);
    } else {
        if (m_textures.count(texAlias) && !m_textures[texAlias].expired()) {
            m_textures[texName] = m_textures[texAlias];
            return m_textures[texAlias].lock();
        }

        std::cout << "Loading texture(GL_TEXTURE_2D) from value [" << defaultValue << "]\n";
        texture = std::make_shared<Texture>(m_textureDefaultWidth, m_textureDefaultHeight, GL_TEXTURE_2D, internalFormat, mipLevels);
        texture->clear(glm::value_ptr(defaultValue), GL_RGBA, GL_FLOAT); // GL_RGBA and GL_FLOAT indicate the format of defaultValue is RGBA float
        m_textures[texAlias] = texture;
    }
    m_textures[texName] = texture;

    return texture;
}

bool is_all_regular_file(const std::vector<fs::path>& paths) {
    for (auto& path : paths) {
        if (!fs::is_regular_file(path)) {
            return false;
        }
    }
    return true;
}

std::shared_ptr<Texture> ResourceManager::load2DTexture(const std::string& texName, const std::vector<fs::path>& texPaths, const glm::vec4& defaultValue, GLenum internalFormat, GLsizei mipLevels, int desiredChannels, bool verticalFlip) {
    std::string texAlias = std::format("default_2d_tex_color({:.3f}, {:.3f}, {:.3f}, {:.3f})", defaultValue.x, defaultValue.y, defaultValue.z, defaultValue.w);
    if (m_textures.count(texName) && !m_textures[texName].expired()) {
        return m_textures[texName].lock();
    }
    
    std::shared_ptr<Texture> texture;
    if (is_all_regular_file(texPaths)) {
        std::vector<std::shared_ptr<Image>> images;
        std::cout << "Loading texture(GL_TEXTURE_2D) from file [";
        for (int i = 0; i < texPaths.size(); i++) {
            auto& texPath = texPaths[i];
            std::cout << texPaths[i] << ", ";
            images.push_back(loadImage(std::format("{}_{}", texName, i), texPath, 1, true));
        }
        std::cout << "]\n";
        auto mimage = Image::merge(images, 4);
        texture = std::make_shared<Texture>(mimage->getWidth(), mimage->getHeight(), GL_TEXTURE_2D, internalFormat, mipLevels);
        texture->upload(mimage);
    } else {
        if (m_textures.count(texAlias) && !m_textures[texAlias].expired()) {
            m_textures[texName] = m_textures[texAlias];
            return m_textures[texAlias].lock();
        }
        
        std::cout << "Loading texture(GL_TEXTURE_2D) from value [" << defaultValue << "]\n";
        texture = std::make_shared<Texture>(m_textureDefaultWidth, m_textureDefaultHeight, GL_TEXTURE_2D, internalFormat, mipLevels);
        texture->clear(glm::value_ptr(defaultValue), GL_RGBA, GL_FLOAT); // GL_RGBA and GL_FLOAT indicate the format of defaultValue is RGBA float
        m_textures[texAlias] = texture;
    }
    m_textures[texName] = texture;
    
    return texture;
}

std::shared_ptr<Texture> ResourceManager::loadCubeTexture(const std::string& texName, const std::vector<fs::path>& texPaths, const glm::vec4& defaultValue, GLenum internalFormat, GLsizei mipLevels, int desiredChannels, bool verticalFlip) {
    std::string texAlias = std::format("default_cube_tex_color({:.3f}, {:.3f}, {:.3f}, {:.3f})", defaultValue.x, defaultValue.y, defaultValue.z, defaultValue.w);
    if (m_textures.count(texName) && !m_textures[texName].expired()) {
        return m_textures[texName].lock();
    }

    std::vector<std::shared_ptr<Image>> images;
    GLsizei width = 1, height = 1;
    if (is_all_regular_file(texPaths)) {
        std::cout << "Loading texture(GL_TEXTURE_CUBE_MAP) from file [";
        for (int i = 0; i < texPaths.size(); i++) {
            auto& texPath = texPaths[i];
            std::cout << texPath << ", ";
            images.push_back(loadImage(std::format("{}_{}", texName, i), texPath, desiredChannels, verticalFlip));
            width  = images.back()->getWidth();
            height = images.back()->getHeight();
        }
        std::cout << "]\n";
    } else {
        if (m_textures.count(texAlias) && !m_textures[texAlias].expired()) {
            m_textures[texName] = m_textures[texAlias];
            return m_textures[texAlias].lock();
        }

        std::cout << "Loading texture(GL_TEXTURE_CUBE_MAP) from value [" << defaultValue << "]\n";
    }

    std::shared_ptr<Texture> texture = std::make_shared<Texture>(width, height, GL_TEXTURE_CUBE_MAP, internalFormat, mipLevels);
    for (auto face = GL_TEXTURE_CUBE_MAP_POSITIVE_X; face <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z; face++) {
        GLint index = face - GL_TEXTURE_CUBE_MAP_POSITIVE_X;
        if (index < images.size() && width == images[index]->getWidth() && height == images[index]->getHeight()) {
            texture->upload(images[index], index, 0);
        } else {
            texture->clear(glm::value_ptr(defaultValue), GL_RGBA, GL_FLOAT); // GL_RGBA and GL_FLOAT indicate the format of defaultValue is RGBA float
        }
    }
    m_textures[texName] = texture;
    if (images.empty()) {
        m_textures[texAlias] = texture;
    }

    return texture;
}

std::shared_ptr<Image> ResourceManager::loadImage(const std::string& imageName, const fs::path& imagePath, int desiredChannels, bool verticalFlip) {
    if (m_images.count(imageName) && !m_images[imageName].expired()) {
        return m_images[imageName].lock();
    }

    auto image = Image::create(imagePath, desiredChannels, verticalFlip); // just use static constructor Image::create()
    m_images[imageName] = image;

    return image;
}

std::shared_ptr<Shader> ResourceManager::loadShader(const std::string& shaderName, const fs::path& vertexShaderPath, const fs::path& fragmentShaderPath) {
    if (m_shaders.count(shaderName) && !m_shaders[shaderName].expired()) {
        return m_shaders[shaderName].lock();
    }
    
    std::shared_ptr<Shader> shader = std::make_shared<Shader>(vertexShaderPath, fragmentShaderPath);
    m_shaders[shaderName] = shader;

    return shader;
}

}; // namespace tinyglrenderer