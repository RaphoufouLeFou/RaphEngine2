#pragma once

#include "export.hpp"
#include <string>

namespace raphEngine {
    class RAPHENGINE_API Serializable
    {
        virtual std::string serialize() = 0;
        virtual bool deserialize(const std::string& input) = 0;
    };
}
