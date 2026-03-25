#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoords;
layout(location = 3) in vec3 aTangent;
layout(location = 4) in vec3 aBitangent;

out VS_OUT{
    vec3 FragPos;
    vec2 TexCoords;
    vec3 FragNormal;
    vec3 TangentLightDir;
    vec3 TangentViewPos;
    vec3 TangentFragPos;
    vec4 FragPosLightSpace;
    mat3 TBN;
} vs_out;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

uniform vec3 lightPos;
uniform vec3 lightDir;
uniform vec3 viewPos;
uniform mat4 lightSpaceMatrix;

void main()
{
    vs_out.FragPos = vec3(model * vec4(aPos, 1.0));
    vs_out.TexCoords = aTexCoords;
    vs_out.FragNormal = aNormal;

    vec3 T = normalize(vec3(model * vec4(aTangent,   0.0)));
    //vec3 B = normalize(vec3(model * vec4(aBitangent, 0.0)));
    vec3 N = normalize(vec3(model * vec4(aNormal,    0.0)));
    T = normalize(T - dot(T, N) * N);

    vec3 B = cross(N, T);

    mat3 TBN = transpose(mat3(T, B, N));

    vs_out.TangentLightDir = TBN * lightDir;
    vs_out.TangentViewPos = viewPos;
    vs_out.TangentFragPos = TBN * vs_out.FragPos;

    vs_out.FragPosLightSpace = lightSpaceMatrix * vec4(vs_out.FragPos, 1.0);

    vs_out.TBN = TBN;

    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
