#pragma once

// GPU backend interface for the material module (the QOpenGLTexture /
// QOpenGLShaderProgram replacement).
//
// The material module never talks to OpenGL directly: it asks this backend
// for texture and program handles, and the renderer installs the real
// implementation with setTextureBackend()/setShaderBackend() once its GL
// context exists. Until then the null backends below answer every call with
// a synthetic handle so the material code runs headless, exactly like the
// Qt-free transport in network/ runs without a socket backend.
//
// The real backend lives outside material/ (the renderer owns the context),
// so it is reported as pending work instead of being written here.

#include "Image.h"

#include <map>
#include <string>

namespace ks::gfx {

enum class ShaderStage { Vertex, Fragment, Geometry };

enum class TextureFilter { Nearest, Linear, LinearMipMapLinear };

enum class TextureWrap { Repeat, ClampToEdge };

// Pixel payload handed to the backend: tightly packed RGBA8, bottom-up data
// already prepared (TextureManager does the vertical flip Qt did with
// QImage::flipped before the upload).
struct TextureDesc {
    int width = 0;
    int height = 0;
    const unsigned char* rgba = nullptr;
    bool srgb = false;
    TextureFilter minFilter = TextureFilter::LinearMipMapLinear;
    TextureFilter magFilter = TextureFilter::Linear;
    TextureWrap wrap = TextureWrap::Repeat;
};

class ITextureBackend {
public:
    virtual ~ITextureBackend() = default;

    // Returns a positive handle, or 0 when the texture could not be created.
    virtual int createTexture(const TextureDesc& desc) = 0;
    virtual void destroyTexture(int handle) = 0;
    virtual bool isValid(int handle) const = 0;
    // Native object for the renderer (QOpenGLTexture* for the OpenGL
    // backend), nullptr for backends that keep no per-texture object.
    virtual void* nativeTexture(int handle) const = 0;
    virtual int width(int handle) const = 0;
    virtual int height(int handle) const = 0;
};

class IShaderBackend {
public:
    virtual ~IShaderBackend() = default;

    // Links vertex+fragment (+optional geometry) sources.
    // Returns a positive handle, or 0 on failure with the driver log in
    // lastError().
    virtual int createProgram(const std::string& name, const std::string& vertexSource,
                              const std::string& fragmentSource,
                              const std::string& geometrySource) = 0;
    virtual void destroyProgram(int handle) = 0;
    // bindProgram()/releaseProgram() must ignore unknown handles (return
    // false) so callers can no-op on stale ids, like the Qt code did with a
    // missing QOpenGLShaderProgram.
    virtual bool bindProgram(int handle) = 0;
    virtual void releaseProgram(int handle) = 0;
    virtual bool setUniformFloat(int handle, const char* name, float value) = 0;
    virtual bool setUniformVec3(int handle, const char* name, float x, float y, float z) = 0;
    // True when the backend has a live rendering context to compile against.
    virtual bool hasContext() const = 0;
    virtual std::string lastError() const = 0;
};

// Null backends: handles are real (so the bookkeeping in TextureManager and
// ShaderManager keeps working) but no GPU object is created.
class NullTextureBackend final : public ITextureBackend {
public:
    int createTexture(const TextureDesc& desc) override;
    void destroyTexture(int handle) override;
    bool isValid(int handle) const override;
    void* nativeTexture(int) const override { return nullptr; }
    int width(int handle) const override;
    int height(int handle) const override;

private:
    struct Entry {
        int width = 0;
        int height = 0;
    };
    std::map<int, Entry> m_textures;
    int m_nextHandle = 1;
};

class NullShaderBackend final : public IShaderBackend {
public:
    int createProgram(const std::string& name, const std::string& vertexSource,
                      const std::string& fragmentSource,
                      const std::string& geometrySource) override;
    void destroyProgram(int handle) override;
    bool bindProgram(int handle) override { return m_bound.count(handle) != 0; }
    void releaseProgram(int handle) override { m_bound.erase(handle); }
    bool setUniformFloat(int, const char*, float) override { return false; }
    bool setUniformVec3(int, const char*, float, float, float) override { return false; }
    bool hasContext() const override { return false; }
    std::string lastError() const override { return m_error; }

private:
    std::map<int, bool> m_programs;
    std::map<int, bool> m_bound;
    int m_nextHandle = 1;
    std::string m_error;
};

// Backend selection. Passing nullptr restores the null backend.
ITextureBackend* textureBackend();
void setTextureBackend(ITextureBackend* backend);
IShaderBackend* shaderBackend();
void setShaderBackend(IShaderBackend* backend);

} // namespace ks::gfx
