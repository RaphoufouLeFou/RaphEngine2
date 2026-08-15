cmake_minimum_required(VERSION 3.21.2)

project(RaphEngine2 VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(FetchContent)

file(GLOB RaphEngine2_SRC
     "src/*.cpp"
     "src/*/*.cpp"
     "src/*/*/*.cpp"
)

set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/imgui)

if(NOT EXISTS ${IMGUI_DIR}/imgui.cpp)
    message(FATAL_ERROR "imgui not found at ${IMGUI_DIR} - did you clone it into the project root?")
endif()

file(GLOB IMGUI_CORE_SRC "${IMGUI_DIR}/*.cpp")

set(IMGUI_BACKEND_SRC
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
    ${IMGUI_DIR}/misc/cpp/imgui_stdlib.cpp
    # ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
    # ${IMGUI_DIR}/backends/imgui_impl_dx11.cpp
    # ${IMGUI_DIR}/backends/imgui_impl_dx12.cpp
    # ${IMGUI_DIR}/backends/imgui_impl_win32.cpp   # needed alongside DX backends on Windows
)

list(APPEND RaphEngine2_SRC ${IMGUI_CORE_SRC} ${IMGUI_BACKEND_SRC})

add_library(RaphEngine2 SHARED ${RaphEngine2_SRC})

target_compile_definitions(RaphEngine2
    PRIVATE
        RAPHENGINE_EXPORTS
        GLFW_INCLUDE_NONE
)

target_compile_features(RaphEngine2 PUBLIC cxx_std_20)

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

if(WIN32)
    target_compile_options(RaphEngine2 PRIVATE /W4 /permissive- /wd4251 /arch:AVX2)
else()
    target_compile_options(RaphEngine2 PRIVATE -Wall -Wextra -Werror -pedantic -fPIC -mavx2 -mfma)
endif()

option(RAPHENGINE2_ENABLE_ASAN "Build RaphEngine2 with AddressSanitizer" ON)

if(RAPHENGINE2_ENABLE_ASAN AND NOT WIN32)
    target_compile_options(RaphEngine2 PRIVATE -fsanitize=address -fno-omit-frame-pointer -g)
    target_link_options(RaphEngine2 PRIVATE -fsanitize=address)
endif()

target_include_directories(RaphEngine2
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/RaphEngine2>
        $<INSTALL_INTERFACE:include>
)

set_target_properties(RaphEngine2 PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
)


# ========================
# dependencies
# ========================

find_package(TBB REQUIRED)
target_link_libraries(RaphEngine2 PUBLIC TBB::tbb)

#imgui

target_include_directories(RaphEngine2
    PUBLIC
        $<BUILD_INTERFACE:${IMGUI_DIR}>
        $<BUILD_INTERFACE:${IMGUI_DIR}/backends>
)

# glm
find_package(glm QUIET)

if(NOT glm_FOUND)
    FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG 0af55ccecd98d4e5a8d1fad7de25ba429d60e863
    )
    FetchContent_MakeAvailable(glm)
endif()

target_link_libraries(RaphEngine2 PRIVATE glm::glm)

# nlohmann json
find_package(nlohmann_json QUIET)

if(nlohmann_json_FOUND)
    target_link_libraries(RaphEngine2 PUBLIC nlohmann_json::nlohmann_json)
else()
    FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.12.0
    )
    FetchContent_MakeAvailable(nlohmann_json)

    target_link_libraries(RaphEngine2 PRIVATE nlohmann_json::nlohmann_json)

    target_include_directories(RaphEngine2 PUBLIC
        $<BUILD_INTERFACE:${nlohmann_json_SOURCE_DIR}/include>
    )
endif()

# GLEW
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

FetchContent_Declare(
    glew
    GIT_REPOSITORY https://github.com/Perlmint/glew-cmake.git
    GIT_TAG glew-cmake-2.2.0
)

FetchContent_MakeAvailable(glew)

target_link_libraries(RaphEngine2 PRIVATE libglew_static)

# GLFW
find_package(glfw3 QUIET)

if(NOT glfw3_FOUND)
    FetchContent_Declare(
        glfw3
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 3.3.9
    )
    FetchContent_MakeAvailable(glfw3)
endif()

target_link_libraries(RaphEngine2 PRIVATE glfw)

# freetype
find_package(Freetype QUIET)

if(NOT Freetype_FOUND)
    FetchContent_Declare(
        freetype
        GIT_REPOSITORY https://github.com/freetype/freetype.git
        GIT_TAG VER-2-13-3
    )
    FetchContent_MakeAvailable(freetype)
endif()

# RmlUi
set(RMLUI_DEBUGGER OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    rmlui
    GIT_REPOSITORY https://github.com/mikke89/RmlUi.git
    GIT_TAG 6.2
)
FetchContent_MakeAvailable(rmlui)

target_link_libraries(RaphEngine2 PRIVATE RmlUi::RmlUi)

set(RMLUI_BACKEND_DIR ${rmlui_SOURCE_DIR}/Backends)

target_sources(RaphEngine2 PRIVATE
    ${RMLUI_BACKEND_DIR}/RmlUi_Platform_GLFW.cpp
    ${RMLUI_BACKEND_DIR}/RmlUi_Renderer_GL3.cpp
)

if(NOT WIN32)
    set_source_files_properties(
        ${RMLUI_BACKEND_DIR}/RmlUi_Platform_GLFW.cpp
        ${RMLUI_BACKEND_DIR}/RmlUi_Renderer_GL3.cpp
        PROPERTIES COMPILE_OPTIONS "-Wno-error"
    )
endif()

target_include_directories(RaphEngine2 PRIVATE ${RMLUI_BACKEND_DIR})
target_compile_definitions(RaphEngine2 PRIVATE
    "RMLUI_GL3_CUSTOM_LOADER=<GL/glew.h>"
)

# stb_image
FetchContent_Declare(
    stb_image
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG master
)

FetchContent_MakeAvailable(stb_image)

target_include_directories(RaphEngine2 PRIVATE ${stb_image_SOURCE_DIR})

# assimp
find_package(assimp QUIET)

if(NOT assimp_FOUND)
    FetchContent_Declare(
        assimp
        GIT_REPOSITORY https://github.com/assimp/assimp.git
        GIT_TAG v5.2.5
    )
    FetchContent_MakeAvailable(assimp)
endif()

target_link_libraries(RaphEngine2 PRIVATE assimp::assimp)

# ========================
# Install
# ========================


install(TARGETS RaphEngine2
    EXPORT RaphEngine2Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/RaphEngine2ConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/RaphEngine2Config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/RaphEngine2Config.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/RaphEngine2
)

install(EXPORT RaphEngine2Targets
    FILE RaphEngine2Targets.cmake
    NAMESPACE RaphEngine2::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/RaphEngine2
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/RaphEngine2Config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/RaphEngine2ConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/RaphEngine2
)

add_subdirectory(editor)
