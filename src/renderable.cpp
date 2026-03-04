#include "RaphEngine2/renderable.hpp"

#include <iostream>
#include <vector>

namespace raphEngine
{
    std::vector<Renderable*> Renderable::render_list_;

    Renderable::Renderable()
    {
        render_list_.push_back(this);
    }
} // namespace raphEngine
