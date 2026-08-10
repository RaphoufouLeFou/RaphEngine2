#version 410 core
layout (location = 0) in vec3 position;
layout (location = 5) in mat4 instanceModel;

void main() 
{
    gl_Position = instanceModel * vec4(position, 1.0); 
}