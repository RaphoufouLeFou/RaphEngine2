#version 410 core
layout(location = 0) in vec3 aPos;

uniform mat4 lightSpaceMatrix; // set once per cascade draw call, not per-instance
uniform mat4 model;

void main()
{
    gl_Position = lightSpaceMatrix * model * vec4(aPos, 1.0);
}