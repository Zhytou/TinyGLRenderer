#pragma once

#include <cstdint>

namespace tinyglrenderer {

struct RendererSetting {
    bool deferred  = false; // deferred rendering enabled or not
    bool ibl       = false; // image based light enabled or not
    bool shadow    = false; // shadow mapping enabled or not
    bool bloom     = false;  // bloom blur enabled or not
    bool lensflare = false;  // lensflare enabled or not
    bool dirtmask  = false;  // dirtmask enabled or not
    bool ssao      = false; // screen space ambient occlusion enabled or not
    bool ssr       = false; // screen space reflection enabled or not
    bool dssr      = true;  // dual screen space refraction enabled or not
    bool taa       = false; // temporal anti aliasing enabled or not

    int x                  = 0;
    int y                  = 0;
    int width              = 1;    // width of current renderer viewport size
    int height             = 1;    // height of current renderer viewport size, namely subwindow of the application window
    int frameWidth         = 2560; // hard-coded width of gbuffer or postprocess frame buffer
    int frameHeight        = 1440; // hard-coded height of gbuffer or postprocess frame buffer
    int skyboxSize         = 1024;
    int brdfLUTSize        = 256;
    int shadowMapSize      = 4096;
    int highlightMapSize   = 1024;
    int bloomMapSize       = 1024; // size of bloom map using dual kawase blur algorithm
    int bloomMipLevels     = 4;    // number of mip levels for bloom map
    int lensflareMapSize   = 512;  // size of lensflare map using gaussian blur algorithm
    int lensflareBlurTimes = 2;    // number of gaussian blur times for lensflare map
};

} // namespace tinyglrenderer
