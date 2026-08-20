#pragma once

#include <RaphEngine2/export.hpp>
#include <string>
#include <memory>

namespace raphEngine::graphics
{
    class RAPHENGINE_API Skybox
    {
    public:
        virtual void set_hdr(const std::string& hdr) = 0;
        virtual void render() = 0;

        static Skybox* getInstance();

    private:
        static std::unique_ptr<Skybox> instance_;
    };
} // namespace raphEngine::graphics
