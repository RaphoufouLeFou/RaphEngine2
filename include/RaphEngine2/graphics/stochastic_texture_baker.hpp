#pragma once

#include <RaphEngine2/export.hpp>

#include <cstdint>
#include <vector>

namespace raphEngine::graphics
{

    struct RAPHENGINE_API StochasticTextureData
    {
        std::vector<uint8_t> gaussianizedPixels; // width * height * channels
        std::vector<uint8_t> inverseLut; // lutResolution * channels
        int width = 0;
        int height = 0;
        int channels = 0;
        int lutResolution = 0;
    };

    RAPHENGINE_API StochasticTextureData
    ComputeStochasticTextureData(const uint8_t* pixels, int width, int height,
                                 int channels, int lutResolution = 256);
} // namespace raphEngine::graphics
