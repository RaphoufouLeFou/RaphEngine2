#include <RaphEngine2/RaphEngine2.hpp>
#include <RaphEngine2/graphics/stochastic_texture_baker.hpp>

#include <algorithm>
#include <cmath>
#include <numbers>
#include <vector>

#include <tbb/parallel_for.h>

namespace raphEngine::graphics
{
    namespace
    {
        constexpr double kGaussianMean = 0.5;
        constexpr double kGaussianStdDev = 1.0 / 6.0;

        double ErfInvSeed(double x)
        {
            double w = -std::log((1.0 - x) * (1.0 + x));
            double p;

            if (w < 5.0)
            {
                w -= 2.5;
                p = 2.81022636e-08;
                p = 3.43273939e-07 + p * w;
                p = -3.5233877e-06 + p * w;
                p = -4.39150654e-06 + p * w;
                p = 0.00021858087 + p * w;
                p = -0.00125372503 + p * w;
                p = -0.00417768164 + p * w;
                p = 0.246640727 + p * w;
                p = 1.50140941 + p * w;
            }
            else
            {
                w = std::sqrt(w) - 3.0;
                p = -0.000200214257;
                p = 0.000100950558 + p * w;
                p = 0.00134934322 + p * w;
                p = -0.00367342844 + p * w;
                p = 0.00573950773 + p * w;
                p = -0.0076224613 + p * w;
                p = 0.00943887047 + p * w;
                p = 1.00167406 + p * w;
                p = 2.83297682 + p * w;
            }

            return p * x;
        }

        double ErfInv(double x)
        {
            x = std::clamp(x, -0.999999, 0.999999);
            double result = ErfInvSeed(x);

            for (int i = 0; i < 2; ++i)
            {
                const double err = std::erf(result) - x;
                result -= err
                    / (2.0 / std::sqrt(std::numbers::pi)
                       * std::exp(-result * result));
            }

            return result;
        }

        double GaussianCdf(double x, double mean, double stdDev)
        {
            return 0.5
                * (1.0 + std::erf((x - mean) / (stdDev * std::sqrt(2.0))));
        }

        double GaussianInverseCdf(double quantile, double mean, double stdDev)
        {
            return mean
                + stdDev * std::sqrt(2.0) * ErfInv(2.0 * quantile - 1.0);
        }

        std::vector<uint32_t> SortPixelsByChannel(const uint8_t* pixels,
                                                  int width, int height,
                                                  int channels, int channel)
        {
            const size_t pixelCount = static_cast<size_t>(width) * height;

            std::vector<uint32_t> pixelIndexByRank(pixelCount);
            for (size_t i = 0; i < pixelCount; ++i)
            {
                pixelIndexByRank[i] = static_cast<uint32_t>(i);
            }

            std::sort(pixelIndexByRank.begin(), pixelIndexByRank.end(),
                      [&](uint32_t a, uint32_t b) {
                          return pixels[a * channels + channel]
                              < pixels[b * channels + channel];
                      });

            return pixelIndexByRank;
        }
    } // namespace

    StochasticTextureData ComputeStochasticTextureData(const uint8_t* pixels,
                                                       int width, int height,
                                                       int channels,
                                                       int lutResolution)
    {
        StochasticTextureData result;
        result.width = width;
        result.height = height;
        result.channels = channels;
        result.lutResolution = lutResolution;

        const size_t pixelCount = static_cast<size_t>(width) * height;
        result.gaussianizedPixels.resize(pixelCount * channels);
        result.inverseLut.resize(static_cast<size_t>(lutResolution) * channels);

        const int channelsToProcess = std::min(channels, 3);

        // Each channel's sort + Gaussianize + LUT build only reads
        // `pixels` and writes to its own channel slot of `result` -- fully
        // independent across channels, safe to run concurrently.
        tbb::parallel_for(0, channelsToProcess, [&](int channel) {
            const std::vector<uint32_t> pixelIndexByRank =
                SortPixelsByChannel(pixels, width, height, channels, channel);

            for (size_t rank = 0; rank < pixelCount; ++rank)
            {
                const uint32_t pixelIndex = pixelIndexByRank[rank];
                const double quantile = (static_cast<double>(rank) + 0.5)
                    / static_cast<double>(pixelCount);
                const double gaussianValue = GaussianInverseCdf(
                    quantile, kGaussianMean, kGaussianStdDev);

                const double clamped = std::clamp(gaussianValue, 0.0, 1.0);
                result.gaussianizedPixels[pixelIndex * channels + channel] =
                    static_cast<uint8_t>(clamped * 255.0 + 0.5);
            }

            for (int lutIndex = 0; lutIndex < lutResolution; ++lutIndex)
            {
                const double gaussianPosition =
                    (static_cast<double>(lutIndex) + 0.5)
                    / static_cast<double>(lutResolution);
                const double quantile = GaussianCdf(
                    gaussianPosition, kGaussianMean, kGaussianStdDev);

                const size_t rank = std::clamp<size_t>(
                    static_cast<size_t>(quantile
                                        * static_cast<double>(pixelCount)),
                    0, pixelCount - 1);
                const uint32_t pixelIndex = pixelIndexByRank[rank];

                result.inverseLut[static_cast<size_t>(lutIndex) * channels
                                  + channel] =
                    pixels[pixelIndex * channels + channel];
            }
        });

        for (int channel = channelsToProcess; channel < channels; ++channel)
        {
            for (size_t i = 0; i < pixelCount; ++i)
            {
                result.gaussianizedPixels[i * channels + channel] =
                    pixels[i * channels + channel];
            }
            for (int lutIndex = 0; lutIndex < lutResolution; ++lutIndex)
            {
                const double t = (static_cast<double>(lutIndex) + 0.5)
                    / static_cast<double>(lutResolution);
                result.inverseLut[static_cast<size_t>(lutIndex) * channels
                                  + channel] =
                    static_cast<uint8_t>(std::clamp(t, 0.0, 1.0) * 255.0 + 0.5);
            }
        }

        return result;
    }
} // namespace raphEngine::graphics
