#version 410 core

layout(location = 0) in vec3 aPos;
layout(location = 2) in vec2 aTexCoords;
layout(location = 5) in mat4 instanceModel;

out vec2 TexCoords;

uniform mat4 lightSpaceMatrix;

void main()
{
    TexCoords = aTexCoords;
    gl_Position = lightSpaceMatrix * instanceModel * vec4(aPos, 1.0);
}
