# This script creates a header file with the shader code as strings

import os

shader_folder = "shaders/"
output_file = "include/RaphEngine2/default_shaders.hpp"
outputCfile = "src/default_shaders.cpp"

# Clear the file
file = open(output_file, "w")
file.truncate(0)
file.write("#pragma once\n\n")
#file.write('#include "RaphEngine.h"\n\n')
file.close()

file = open(outputCfile, "w")
file.truncate(0)

file.write("#include \"default_shaders.hpp\"\n")
file.close()

for shader in os.listdir(shader_folder):
    if shader.endswith(".fs"):
        with open(shader_folder + shader, "r", encoding="utf8") as file:
            shader_code = file.read()
            shader_name = shader.replace(".fs", "").replace(" ", "_").replace(".", "_").replace("-", "_") + "_FS_shader"   
        with open(output_file, "a", encoding="utf8") as output:
            output.write("inline const char* " + shader_name + " = R\"(\n" + shader_code + "\n)\";\n\n")
    
    if shader.endswith(".vs"):
        with open(shader_folder + shader, "r", encoding="utf8") as file:
            shader_code = file.read()
            shader_name = shader.replace(".vs", "").replace(" ", "_").replace(".", "_").replace("-", "_") + "_VS_shader"
        with open(output_file, "a", encoding="utf8") as output:
            output.write("inline const char* " + shader_name + " = R\"(\n" + shader_code + "\n)\";\n\n")

    if shader.endswith(".gs"):
        with open(shader_folder + shader, "r", encoding="utf8") as file:
            shader_code = file.read()
            shader_name = shader.replace(".gs", "").replace(" ", "_").replace(".", "_").replace("-", "_") + "_GS_shader"
        with open(output_file, "a", encoding="utf8") as output:
            output.write("inline const char* " + shader_name + " = R\"(\n" + shader_code + "\n)\";\n\n")

    if shader.endswith(".glsl"):
        with open(shader_folder + shader, "r", encoding="utf8") as file:
            shader_code = file.read()
            shader_name = shader.replace(".glsl", "").replace(" ", "_").replace(".", "_").replace("-", "_") + "_shader"        
        with open(output_file, "a", encoding="utf8") as output:
            output.write("inline const char* " + shader_name + " = R\"(\n" + shader_code + "\n)\";\n\n")
