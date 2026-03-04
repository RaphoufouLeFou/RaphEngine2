#pragma once

#include <RaphEngine2/export.hpp>

namespace raphEngine::graphics {
    struct RAPHENGINE_API Resolution
    {

        Resolution(unsigned short res_x_ = 1920, unsigned short res_y_ = 1080)
            : res_x(res_x_)
            , res_y(res_y_)
        {}

        unsigned short res_x;
        unsigned short res_y;
    };
}