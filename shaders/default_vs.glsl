#version 410 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoords;
layout(location = 3) in vec3 aTangent;
layout(location = 4) in vec3 aBitangent;

out VS_OUT
{
    vec3 FragPos; // world space
    vec2 TexCoords;
    vec3 FragNormal; // world space
    vec3 TangentLightDir; // tangent space
    vec3 TangentViewPos; // tangent space
    vec3 TangentFragPos; // tangent space
    mat3 TBN; // tangent → world
}
vs_out;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
uniform vec3 lightDir; // world space directional light
uniform vec3 viewPos; // world space camera position

void main()
{
    vs_out.FragPos = vec3(model * vec4(aPos, 1.0));
    vs_out.TexCoords = aTexCoords;

    // FIX 1: Use the normal matrix to correctly handle non-uniform scaling.
    //         Previously aNormal was passed raw (object space), breaking bias
    //         and non-normal-map lighting entirely.
    mat3 normalMatrix = transpose(inverse(mat3(model)));

    vec3 N = normalize(normalMatrix * aNormal);
    vec3 T = normalize(normalMatrix * aTangent);
    T = normalize(T - dot(T, N) * N); // Gram-Schmidt re-orthogonalization
    vec3 B = cross(N, T);

    // FIX 2: TBN is tangent → world (the natural convention).
    //         Previously it was stored transposed (world → tangent), which
    //         forced the fragment shader to call transpose(TBN) to use it
    //         normally — confusing and error-prone.
    mat3 TBN = mat3(T, B, N); // tangent → world
    mat3 TBN_T = transpose(TBN); // world   → tangent

    vs_out.FragNormal = N;
    vs_out.TBN = TBN; // passed to FS to convert tangent normal → world for bias
    vs_out.TangentLightDir = TBN_T * lightDir;
    vs_out.TangentViewPos = TBN_T * viewPos; // FIX 3: was missing TBN_T *
    vs_out.TangentFragPos = TBN_T * vs_out.FragPos;

    // NOTE: lightPos, lightSpaceMatrix, FragPosLightSpace removed — all dead
    //       code left over from the pre-CSM implementation.

    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
