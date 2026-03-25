#include "settings/settings.hpp"

namespace raphEngine::settings
{
    const std::string& Settings::get_pretty_name() const
    {
        return setting_name_;
    }
} // namespace raphEngine::settings
