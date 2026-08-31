#include <RaphEngine2/audio/audio.hpp>
#include <RaphEngine2/audio/sound.hpp>

#include <memory>
#include <utility>

#include <miniaudio.h>

namespace raphEngine
{

    struct Sound::Impl
    {
        ma_sound sound{};
        bool loaded = false;
    };

    Sound::Sound()
        : m_impl(std::make_unique<Impl>())
    {}

    Sound::~Sound()
    {
        Unload();
    }

    Sound::Sound(Sound&& other) noexcept = default;

    Sound& Sound::operator=(Sound&& other) noexcept
    {
        if (this != &other)
        {
            Unload();
            m_impl = std::move(other.m_impl);
        }
        return *this;
    }

    bool Sound::Load(const std::string& filepath, bool stream, bool spatial)
    {
        Unload();

        if (!m_impl)
        {
            m_impl = std::make_unique<Impl>();
        }

        ma_engine* engine = static_cast<ma_engine*>(Audio::GetEngine());
        if (!engine)
        {
            return false;
        }

        ma_uint32 flags = stream ? MA_SOUND_FLAG_STREAM : MA_SOUND_FLAG_DECODE;
        if (!spatial)
        {
            flags |= MA_SOUND_FLAG_NO_SPATIALIZATION;
        }

        ma_result result = ma_sound_init_from_file(
            engine, filepath.c_str(), flags, nullptr, nullptr, &m_impl->sound);
        if (result != MA_SUCCESS)
        {
            return false;
        }

        m_impl->loaded = true;
        return true;
    }

    void Sound::Unload()
    {
        if (m_impl && m_impl->loaded)
        {
            ma_sound_uninit(&m_impl->sound);
            m_impl->loaded = false;
        }
    }

    void Sound::Play()
    {
        if (m_impl && m_impl->loaded)
        {
            ma_sound_start(&m_impl->sound);
        }
    }

    void Sound::Stop()
    {
        if (m_impl && m_impl->loaded)
        {
            ma_sound_stop(&m_impl->sound);
        }
    }

    void Sound::Rewind()
    {
        if (m_impl && m_impl->loaded)
        {
            ma_sound_seek_to_pcm_frame(&m_impl->sound, 0);
        }
    }

    bool Sound::IsValid() const
    {
        return m_impl && m_impl->loaded;
    }

    bool Sound::IsPlaying() const
    {
        return m_impl && m_impl->loaded
            && ma_sound_is_playing(&m_impl->sound) != 0;
    }

    bool Sound::IsLooping() const
    {
        return m_impl && m_impl->loaded
            && ma_sound_is_looping(&m_impl->sound) != 0;
    }

    bool Sound::IsAtEnd() const
    {
        return !m_impl || !m_impl->loaded
            || ma_sound_at_end(&m_impl->sound) != 0;
    }

    void Sound::SetVolume(float volume)
    {
        if (m_impl && m_impl->loaded)
        {
            ma_sound_set_volume(&m_impl->sound, volume);
        }
    }

    void Sound::SetPitch(float pitch)
    {
        if (m_impl && m_impl->loaded)
        {
            ma_sound_set_pitch(&m_impl->sound, pitch);
        }
    }

    void Sound::SetLooping(bool looping)
    {
        if (m_impl && m_impl->loaded)
        {
            ma_sound_set_looping(&m_impl->sound, looping);
        }
    }

    void Sound::SetPosition(const glm::vec3& position)
    {
        if (m_impl && m_impl->loaded)
        {
            ma_sound_set_position(&m_impl->sound, position.x, position.y,
                                  position.z);
        }
    }

    void Sound::SetVelocity(const glm::vec3& velocity)
    {
        if (m_impl && m_impl->loaded)
        {
            ma_sound_set_velocity(&m_impl->sound, velocity.x, velocity.y,
                                  velocity.z);
        }
    }

    void Sound::SetMinDistance(float distance)
    {
        if (m_impl && m_impl->loaded)
        {
            ma_sound_set_min_distance(&m_impl->sound, distance);
        }
    }

    void Sound::SetMaxDistance(float distance)
    {
        if (m_impl && m_impl->loaded)
        {
            ma_sound_set_max_distance(&m_impl->sound, distance);
        }
    }

    void Sound::SetRolloff(float rolloff)
    {
        if (m_impl && m_impl->loaded)
        {
            ma_sound_set_rolloff(&m_impl->sound, rolloff);
        }
    }

} // namespace raphEngine
