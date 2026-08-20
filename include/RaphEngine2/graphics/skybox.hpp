#pragma once

#include <RaphEngine2/export.hpp>
#include <vector>
#include <string>
#include <memory>

namespace raphEngine::graphics
{
    class RAPHENGINE_API Skybox
    {
    public:
        virtual void set_faces(const std::vector<std::string>& faces) = 0;
        virtual void render() = 0;

        static Skybox* getInstance();
    private:
        static std::unique_ptr<Skybox> instance_;
    };
}