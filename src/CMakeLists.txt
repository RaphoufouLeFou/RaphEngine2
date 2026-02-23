add_executable(RapEngineUI)

set_target_properties(RapEngineUI PROPERTIES CXX_STANDARD 20)
target_compile_options(RapEngineUI PRIVATE -Wall -Wextra -Werror -pedantic -Wold-style-cast)

add_subdirectory("src/")
target_include_directories(RapEngineUI PUBLIC "include/")
