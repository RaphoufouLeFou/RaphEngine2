
#include "scenes/reflection.hpp"

namespace raphEngine::reflection
{

    std::unordered_map<std::type_index, TypeInfo>& Reflection::registry()
    {
        static std::unordered_map<std::type_index, TypeInfo> table;
        return table;
    }

} // namespace raphEngine::reflection
