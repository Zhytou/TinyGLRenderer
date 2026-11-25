#include "shader.hpp"

#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <vector>

#include "utils.hpp"

namespace tinyglrenderer {

namespace fs = std::filesystem;

Shader::Shader(const fs::path& vertShaderPath, const fs::path& fragShaderPath) {
    // std::cout << "Compiling GLSL shader [" << vertShaderPath << "] and [" << fragShaderPath << "]\n";

    // compile and link shaders
    m_filepath = { fs::canonical(vertShaderPath).string(), fs::canonical(fragShaderPath).string() };
    m_source   = { precompile(vertShaderPath), precompile(fragShaderPath) };
    m_id       = link({
        compile(m_source.first, GL_VERTEX_SHADER),
        compile(m_source.second, GL_FRAGMENT_SHADER),
    });

    // generate binding point layout for attributes,uniformbuffers and textures
    introspect();
}

Shader::~Shader() {
    if (m_id != 0) { glDeleteProgram(m_id); }
}

void Shader::use() const {
    if (m_id != 0) { glUseProgram(m_id); }
}

GLint Shader::getUniformLocation(const std::string& name) {
    if (m_locations.count(name) == 0) { m_locations[name] = glGetUniformLocation(m_id, name.c_str()); }
    return m_locations[name];
}

std::string Shader::precompile(const fs::path& shaderPath) {
    std::ifstream file(shaderPath.string());
    std::stringstream buffer;
    std::string line;
    fs::path baseDir = shaderPath.parent_path();

    if (!fs::exists(shaderPath)) { throw std::runtime_error("Shader::compile: Shader source file not found: " + shaderPath.string()); }
    if (!file.is_open()) { throw std::runtime_error("Shader::compile: Failed to open: " + shaderPath.string()); }

    while (std::getline(file, line)) {
        size_t pos = line.find("#include \"");
        if (pos != std::string::npos) {
            size_t start = pos + 10;
            size_t end   = line.find("\"", start);
            if (end != std::string::npos) {
                fs::path includePath = baseDir / line.substr(start, end - start);
                buffer << precompile(includePath);
                // std::cout << "Including GLSL script[" << includePath << "]\n";
            }
        } else {
            buffer << line << "\n";
        }
    }
    return buffer.str();
}

GLuint Shader::compile(const std::string& source, GLenum type) {
    GLuint shader           = glCreateShader(type);
    const GLchar* sourcePtr = source.c_str();
    glShaderSource(shader, 1, &sourcePtr, nullptr);
    glCompileShader(shader);

    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE) {
        GLsizei infoLogSize;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogSize);
        std::vector<GLchar> infoLog(infoLogSize);
        glGetShaderInfoLog(shader, infoLogSize, nullptr, infoLog.data());
        throw std::runtime_error("Shader::compile: Shader compilation failed\n" + std::string(infoLog.data()));
    }
    return shader;
}

GLuint Shader::link(const std::initializer_list<GLuint>& shaders) {
    GLuint program = glCreateProgram();

    for (GLuint shader : shaders) { glAttachShader(program, shader); }
    glLinkProgram(program);
    for (GLuint shader : shaders) {
        glDetachShader(program, shader);
        glDeleteShader(shader);
    }

    GLint status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (status == GL_TRUE) {
        glValidateProgram(program);
        glGetProgramiv(program, GL_VALIDATE_STATUS, &status);
    }
    if (status != GL_TRUE) {
        GLsizei infoLogSize;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogSize);
        std::vector<GLchar> infoLog(infoLogSize);
        glGetProgramInfoLog(program, infoLogSize, nullptr, infoLog.data());
        throw std::runtime_error("Shader::link: Program link failed\n" + std::string(infoLog.data()));
    }
    return program;
}

void Shader::introspect() {
    GLint numUniforms = 0;
    glGetProgramInterfaceiv(m_id, GL_UNIFORM, GL_ACTIVE_RESOURCES, &numUniforms);

    GLenum properties[] = { GL_NAME_LENGTH, GL_TYPE, GL_LOCATION };

    for (int i = 0; i < numUniforms; ++i) {
        GLint results[3];
        glGetProgramResourceiv(m_id, GL_UNIFORM, i, 3, properties, 3, nullptr, results);

        GLint nameLength = results[0];
        std::vector<char> nameBuffer(nameLength);
        glGetProgramResourceName(m_id, GL_UNIFORM, i, nameLength, nullptr, nameBuffer.data());
        std::string uniformName(nameBuffer.data(), nameLength - 1);

        GLenum target  = results[1];
        GLint location = results[2];

        if (target == GL_SAMPLER_2D || target == GL_SAMPLER_CUBE) {
            GLint bindingSlot = -1;
            glGetUniformiv(m_id, location, &bindingSlot);

            m_bindings[uniformName] = bindingSlot;
        }
    }
}

} // namespace tinyglrenderer
