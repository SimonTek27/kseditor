#include "GfxBackend.h"

namespace ks::gfx {

namespace {

NullTextureBackend g_nullTextures;
NullShaderBackend g_nullShaders;

ITextureBackend* g_textures = &g_nullTextures;
IShaderBackend* g_shaders = &g_nullShaders;

} // namespace

int NullTextureBackend::createTexture(const TextureDesc& desc)
{
    if (desc.width <= 0 || desc.height <= 0) return 0;
    const int handle = m_nextHandle++;
    m_textures[handle] = Entry{desc.width, desc.height};
    return handle;
}

void NullTextureBackend::destroyTexture(int handle) { m_textures.erase(handle); }

bool NullTextureBackend::isValid(int handle) const { return m_textures.count(handle) != 0; }

int NullTextureBackend::width(int handle) const
{
    auto it = m_textures.find(handle);
    return it != m_textures.end() ? it->second.width : 0;
}

int NullTextureBackend::height(int handle) const
{
    auto it = m_textures.find(handle);
    return it != m_textures.end() ? it->second.height : 0;
}

int NullShaderBackend::createProgram(const std::string&, const std::string&,
                                     const std::string&, const std::string& geometrySource)
{
    // Same contract the Qt code had: without a live GL context the compile
    // fails. Textures succeed because they only need memory, shaders need a
    // driver.
    if (!geometrySource.empty()) m_error = "geometry shader: no OpenGL context";
    else m_error = "no OpenGL context";
    return 0;
}

void NullShaderBackend::destroyProgram(int handle)
{
    m_programs.erase(handle);
    m_bound.erase(handle);
}

ITextureBackend* textureBackend() { return g_textures; }

void setTextureBackend(ITextureBackend* backend) { g_textures = backend ? backend : &g_nullTextures; }

IShaderBackend* shaderBackend() { return g_shaders; }

void setShaderBackend(IShaderBackend* backend) { g_shaders = backend ? backend : &g_nullShaders; }

} // namespace ks::gfx
