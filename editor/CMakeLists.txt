file(GLOB RaphEditor_SRC
     "src/*.cpp"
     "src/*/*.cpp"
)

add_executable(RaphEditor ${RaphEditor_SRC})

set_target_properties(RaphEditor PROPERTIES CXX_STANDARD 20)
target_compile_options(RaphEditor PRIVATE -Wall -Wextra -Werror -pedantic -Wold-style-cast)

target_include_directories(RaphEditor PUBLIC "include/")


target_link_libraries(RaphEditor PRIVATE RaphEngine2)
