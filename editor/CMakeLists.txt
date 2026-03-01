file(GLOB RaphEditor_SRC
     "src/*.cpp"
     "src/*/*.cpp"
)

add_executable(RaphEditor ${RaphEditor_SRC})

set_target_properties(RaphEditor PROPERTIES CXX_STANDARD 20)

if(WIN32)
    target_compile_options(RaphEditor PRIVATE /W4 /permissive- /wd4251)
else()
    target_compile_options(RaphEditor PRIVATE -Wall -Wextra -Werror -pedantic -Wold-style-cast)
endif()

# Copies the DLL into the same folder as the executable for win builds
if(WIN32)
    add_custom_command(TARGET RaphEditor POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:RaphEngine2>
            $<TARGET_FILE_DIR:RaphEditor>
    )
endif()

# =========================
# dependencies
# =========================

find_package(glm QUIET)

if(NOT glm_FOUND)
    FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG 0af55ccecd98d4e5a8d1fad7de25ba429d60e863
    )
    FetchContent_MakeAvailable(glm)
endif()

target_link_libraries(RaphEditor PRIVATE glm::glm)


target_include_directories(RaphEditor PUBLIC "include/")
target_include_directories(RaphEditor PUBLIC "../include/")
target_link_libraries(RaphEditor PRIVATE RaphEngine2)
