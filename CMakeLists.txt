cmake_minimum_required(VERSION 3.21.2)

project(RaphEngine2 VERSION 1.0.0 LANGUAGES CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(FetchContent)

file(GLOB RaphEngine2_SRC
     "src/*.cpp"
     "src/*/*.cpp"
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
    target_compile_options(RaphEngine2 PRIVATE -Wall -Wextra -Werror -pedantic -Wold-style-cast)
endif()

target_include_directories(RaphEngine2
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

set_target_properties(RaphEngine2 PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
)


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

target_link_libraries(RaphEngine2 PRIVATE glm::glm)

# =========================
# Install
# =========================


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

#add_subdirectory(editor)