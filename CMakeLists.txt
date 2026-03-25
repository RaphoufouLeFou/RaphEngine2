cmake_minimum_required(VERSION 3.21.2)

project(RaphEngine2 VERSION 1.0.0 LANGUAGES CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(FetchContent)

file(GLOB RaphEngine2_SRC
     "src/*.cpp"
     "src/*/*.cpp"
     "src/*/*/*.cpp"
)

add_library(RaphEngine2 SHARED ${RaphEngine2_SRC})

target_compile_definitions(RaphEngine2
    PRIVATE
        RAPHENGINE_EXPORTS
)

target_compile_features(RaphEngine2 PUBLIC cxx_std_20)

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

if(WIN32)
    target_compile_options(RaphEngine2 PRIVATE /W4 /permissive- /wd4251)
else()
    target_compile_options(RaphEngine2 PRIVATE -Wall -Wextra -Werror -pedantic -fPIC)
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
