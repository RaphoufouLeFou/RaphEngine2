#pragma once

#include <RaphEngine2/export.hpp>
#include <vector>
#include <memory>

namespace raphEngine
{
    class RAPHENGINE_API Renderable
    {
    public:
        Renderable();
        virtual void render() = 0;
    private:
        static std::vector<Renderable *> render_list_;
    };
}
