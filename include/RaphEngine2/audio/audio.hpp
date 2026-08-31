#pragma once

#include <cstdint>
#include <string>

#include <glm/glm.hpp>

#include <RaphEngine2/RaphEngine2.hpp>
#include <RaphEngine2/audio/sound.hpp>

namespace raphEngine
{

    class RAPHENGINE_API Audio
    {
    public:
        Audio() = delete;

        static bool Init(uint32_t listenerCount = 1);
        static void Shutdown();
        static bool IsInitialized();

        static void Update();

        static void SetMasterVolume(float volume);

        static void SetListenerPosition(const glm::vec3& position,
                                        uint32_t listenerIndex = 0);
        static void SetListenerDirection(const glm::vec3& forward,
                                         uint32_t listenerIndex = 0);
        static void SetListenerWorldUp(const glm::vec3& up,
                                       uint32_t listenerIndex = 0);
        static void SetListenerVelocity(const glm::vec3& velocity,
                                        uint32_t listenerIndex = 0);

        static void PlaySound(const std::string& filepath, float volume = 1.0f);
        static void PlaySound3D(const std::string& filepath,
                                const glm::vec3& position, float volume = 1.0f);

        static void* GetEngine();
    };

} // namespace raphEngine
