#include <RaphEngine2/audio/audio.hpp>
#include <RaphEngine2/audio/sound.hpp>

#include <memory>
#include <utility>
#include <vector>

#include <miniaudio.h>

namespace raphEngine
{

    namespace
    {
        ma_engine g_engine{};
        bool g_initialized = false;
        std::vector<std::unique_ptr<Sound>> g_oneShots;
    } // namespace

    bool Audio::Init(uint32_t listenerCount)
    {
        if (g_initialized)
        {
            return true;
        }

        ma_engine_config config = ma_engine_config_init();
        config.listenerCount = listenerCount;

        ma_result result = ma_engine_init(&config, &g_engine);
        if (result != MA_SUCCESS)
        {
            return false;
        }

        g_initialized = true;
        return true;
    }

    void Audio::Shutdown()
    {
        if (!g_initialized)
        {
            return;
        }

        g_oneShots.clear();
        ma_engine_uninit(&g_engine);
        g_initialized = false;
    }

    bool Audio::IsInitialized()
    {
        return g_initialized;
    }

    void Audio::Update()
    {
        for (auto it = g_oneShots.begin(); it != g_oneShots.end();)
        {
            if ((*it)->IsAtEnd())
            {
                it = g_oneShots.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    void Audio::SetMasterVolume(float volume)
    {
        if (g_initialized)
        {
            ma_engine_set_volume(&g_engine, volume);
        }
    }

    void Audio::SetListenerPosition(const glm::vec3& position,
                                    uint32_t listenerIndex)
    {
        if (g_initialized)
        {
            ma_engine_listener_set_position(&g_engine, listenerIndex,
                                            position.x, position.y, position.z);
        }
    }

    void Audio::SetListenerDirection(const glm::vec3& forward,
                                     uint32_t listenerIndex)
    {
        if (g_initialized)
        {
            ma_engine_listener_set_direction(&g_engine, listenerIndex,
                                             forward.x, forward.y, forward.z);
        }
    }

    void Audio::SetListenerWorldUp(const glm::vec3& up, uint32_t listenerIndex)
    {
        if (g_initialized)
        {
            ma_engine_listener_set_world_up(&g_engine, listenerIndex, up.x,
                                            up.y, up.z);
        }
    }

    void Audio::SetListenerVelocity(const glm::vec3& velocity,
                                    uint32_t listenerIndex)
    {
        if (g_initialized)
        {
            ma_engine_listener_set_velocity(&g_engine, listenerIndex,
                                            velocity.x, velocity.y, velocity.z);
        }
    }

    void Audio::PlaySound(const std::string& filepath, float volume)
    {
        if (!g_initialized)
        {
            return;
        }

        auto sound = std::make_unique<Sound>();
        if (sound->Load(filepath, false, false))
        {
            sound->SetVolume(volume);
            sound->Play();
            g_oneShots.push_back(std::move(sound));
        }
    }

    void Audio::PlaySound3D(const std::string& filepath,
                            const glm::vec3& position, float volume)
    {
        if (!g_initialized)
        {
            return;
        }

        auto sound = std::make_unique<Sound>();
        if (sound->Load(filepath, false, true))
        {
            sound->SetVolume(volume);
            sound->SetPosition(position);
            sound->Play();
            g_oneShots.push_back(std::move(sound));
        }
    }

    void* Audio::GetEngine()
    {
        return g_initialized ? &g_engine : nullptr;
    }

} // namespace raphEngine
