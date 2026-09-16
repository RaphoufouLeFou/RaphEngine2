#include <RaphEngine2/RaphEngine2.hpp>
#include <RaphEngine2/terrain/noise.hpp>

#include <algorithm>
#include <cmath>
#include <random>

namespace raphEngine::terrain
{
    namespace
    {
        float Fade(float t)
        {
            return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
        }

        float Lerp(float t, float a, float b)
        {
            return a + t * (b - a);
        }

        float Grad(int hash, float x, float y, float z)
        {
            const int h = hash & 15;
            const float u = h < 8 ? x : y;
            const float v = h < 4 ? y : (h == 12 || h == 14) ? x : z;
            return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
        }
    } // namespace

    PerlinNoise::PerlinNoise(uint32_t seed)
    {
        std::array<int, 256> p{};
        for (int i = 0; i < 256; ++i)
        {
            p[i] = i;
        }

        std::mt19937 rng(seed);
        std::shuffle(p.begin(), p.end(), rng);

        for (int i = 0; i < 256; ++i)
        {
            permutation_[i] = p[i];
            permutation_[i + 256] = p[i];
        }
    }

    float PerlinNoise::Sample(float x, float y) const noexcept
    {
        constexpr float z = 0.0f;

        const int X = static_cast<int>(std::floor(x)) & 255;
        const int Y = static_cast<int>(std::floor(y)) & 255;
        const int Z = static_cast<int>(std::floor(z)) & 255;

        const float xf = x - std::floor(x);
        const float yf = y - std::floor(y);
        const float zf = z - std::floor(z);

        const float u = Fade(xf);
        const float v = Fade(yf);
        const float w = Fade(zf);

        const int A = permutation_[X] + Y;
        const int AA = permutation_[A] + Z;
        const int AB = permutation_[A + 1] + Z;
        const int B = permutation_[X + 1] + Y;
        const int BA = permutation_[B] + Z;
        const int BB = permutation_[B + 1] + Z;

        return Lerp(
            w,
            Lerp(v,
                 Lerp(u, Grad(permutation_[AA], xf, yf, zf),
                      Grad(permutation_[BA], xf - 1, yf, zf)),
                 Lerp(u, Grad(permutation_[AB], xf, yf - 1, zf),
                      Grad(permutation_[BB], xf - 1, yf - 1, zf))),
            Lerp(v,
                 Lerp(u, Grad(permutation_[AA + 1], xf, yf, zf - 1),
                      Grad(permutation_[BA + 1], xf - 1, yf, zf - 1)),
                 Lerp(u, Grad(permutation_[AB + 1], xf, yf - 1, zf - 1),
                      Grad(permutation_[BB + 1], xf - 1, yf - 1, zf - 1))));
    }

    float SampleFractalNoise(const PerlinNoise& noise, glm::vec2 worldXY,
                             const FractalNoiseParams& params) noexcept
    {
        float amplitude = 1.0f;
        float frequency = 1.0f / params.baseFeatureScale;
        float sum = 0.0f;
        float maxAmplitude = 0.0f;

        for (int i = 0; i < params.octaves; ++i)
        {
            sum += noise.Sample(worldXY.x * frequency, worldXY.y * frequency)
                * amplitude;
            maxAmplitude += amplitude;

            amplitude *= params.persistence;
            frequency *= params.lacunarity;
        }

        return maxAmplitude > 0.0f ? sum / maxAmplitude : 0.0f;
    }
} // namespace raphEngine::terrain
