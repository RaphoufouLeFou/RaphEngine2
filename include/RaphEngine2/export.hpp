#pragma once

#ifdef _WIN32
#    ifdef RAPHENGINE_EXPORTS
#        define RAPHENGINE_API __declspec(dllexport)
#    else
#        define RAPHENGINE_API __declspec(dllimport)
#    endif
#else
#    ifdef RAPHENGINE_EXPORTS
#        define RAPHENGINE_API __attribute__((visibility("default")))
#    else
#        define RAPHENGINE_API
#    endif
#endif
