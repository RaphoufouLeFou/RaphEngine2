#pragma once

#include <RaphEngine2/export.hpp>
#include <vector>

namespace raphEngine
{
    class RAPHENGINE_API Renderable
    {
    public:
        Renderable();
        virtual void render() const = 0;

    private:
        static std::vector<Renderable*> render_list_;
    };
} // namespace raphEngine
