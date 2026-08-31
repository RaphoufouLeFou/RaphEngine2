#pragma once

#include <memory>
#include <string>

#include <glm/glm.hpp>
#include <RaphEngine2/RaphEngine2.hpp>

namespace raphEngine
{

    class RAPHENGINE_API Sound
    {
    public:
        Sound();
        ~Sound();

        Sound(const Sound&) = delete;
        Sound& operator=(const Sound&) = delete;

        Sound(Sound&& other) noexcept;
        Sound& operator=(Sound&& other) noexcept;

        bool Load(const std::string& filepath, bool stream = false,
                  bool spatial = true);
        void Unload();

        void Play();
        void Stop();
        void Rewind();

        bool IsValid() const;
        bool IsPlaying() const;
        bool IsLooping() const;
        bool IsAtEnd() const;

        void SetVolume(float volume);
        void SetPitch(float pitch);
        void SetLooping(bool looping);

        void SetPosition(const glm::vec3& position);
        void SetVelocity(const glm::vec3& velocity);
        void SetMinDistance(float distance);
        void SetMaxDistance(float distance);
        void SetRolloff(float rolloff);

    private:
        struct Impl;
        std::unique_ptr<Impl> m_impl;
    };

} // namespace raphEngine
