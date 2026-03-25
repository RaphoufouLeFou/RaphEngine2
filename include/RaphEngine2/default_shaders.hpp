#pragma once

inline const char* default_fs_shader = R"(
#version 330 core

#define SAMPLES_COUNT 1
#define INV_SAMPLES_COUNT (1.0f / SAMPLES_COUNT)
#define PIXELS_COUNT ((SAMPLES_COUNT * 2 + 1) * (SAMPLES_COUNT * 2 + 1))
out vec4 FragColor;

in VS_OUT{
    vec3 FragPos;
    vec2 TexCoords;
    vec3 FragNormal;
    vec3 TangentLightDir;
    vec3 TangentViewPos;
    vec3 TangentFragPos;
    vec4 FragPosLightSpace;
    mat3 TBN;
} fs_in;

uniform sampler2D texture_diffuse;
uniform sampler2D texture_specular;
uniform sampler2D texture_normal;
uniform sampler2D texture_height;
uniform sampler2DArray shadowMap;
uniform sampler3D gShadowMapOffsetTexture;

uniform int gShadowMapFilterSize = 0;
uniform float gShadowMapOffsetTextureSize;
uniform float gShadowMapOffsetFilterSize;
uniform float gShadowMapRandomRadius = 0.0;

uniform vec3 lightPos;
uniform vec3 lightDir;
uniform vec3 viewPos;
uniform float heightScale;
uniform float farPlane;
uniform bool HaveTexture;
uniform bool HaveNormalMap;
uniform bool HaveSpecularMap;
uniform bool HaveHeightMap;
uniform vec2 offsets[PIXELS_COUNT];
uniform mat4 view;

layout (std140) uniform LightSpaceMatrices
{
    mat4 lightSpaceMatrices[8];
};
uniform float cascadePlaneDistances[8];
uniform int cascadeCount;   // number of frusta - 1

float ShadowCalculation(vec3 fragPosWorldSpace, vec3 normal)
{
    // select cascade layer
    vec4 fragPosViewSpace = view * vec4(fragPosWorldSpace, 1.0);
    float depthValue = abs(fragPosViewSpace.z);

    int layer = -1;
    for (int i = 0; i < cascadeCount; ++i)
    {
        if (depthValue < farPlane / cascadePlaneDistances[i])
        {
            layer = i;
            break;
        }
    }
    //layer = 4;
    if (layer == -1)
    {
        layer = cascadeCount;
    }

    vec4 fragPosLightSpace = lightSpaceMatrices[layer] * vec4(fragPosWorldSpace, 1.0);
    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;

    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;

    // keep the shadow at 0.0 when outside the far_plane region of the light's frustum.
    if (currentDepth > 1.0)
    {
        return 0.0;
    }
    // calculate bias (based on depth map resolution and slope)
    float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);  
    if (layer < 2)
    {
        bias *= 0.015;
    }
    else
    {
        bias *= 0.008; //1 / ((farPlane / cascadePlaneDistances[layer]) * biasModifier);
    }

    // PCF
    float shadow = 0.0;
    vec2 texelSize = 1.0 / vec2(textureSize(shadowMap, 0));
    for(int x = -1; x <= 1; x++)
    {
        for(int y = -1; y <= 1; y++)
        {
            float pcfDepth = texture(shadowMap, vec3(projCoords.xy + vec2(x, y) * texelSize, layer)).r;
            shadow += (currentDepth - bias) > pcfDepth ? 1.0 : 0.0;        
        }    
    }
    shadow /= 9.0;
        
    return shadow;
}

void main()
{

    vec3 viewDir = normalize(fs_in.TangentViewPos - fs_in.FragPos);
    vec3 normal = fs_in.FragNormal;
    
    if (HaveNormalMap)
	{
        normal = texture(texture_normal, fs_in.TexCoords).rgb;
        normal = normalize(fs_in.TBN * (normal * 2.0 - 1.0)); 
	}
    
    vec3 color = vec3(191, 64, 191) / 255.0; // default color
    if (HaveTexture)
        color = texture(texture_diffuse, fs_in.TexCoords).rgb;
    
    vec3 lightColor = vec3(1.0);

    // ambient
    vec3 ambient = 0.1 * lightColor;
    // diffuse
    vec3 lightDir2 = fs_in.TangentLightDir;
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * lightColor;
    // specular
    //vec3 reflectDir = reflect(-lightDir, normal);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float specFact = 1; 
    if(HaveSpecularMap)
        specFact = texture(texture_specular, fs_in.TexCoords).r;
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0) * specFact;
    //vec3 specular = vec3(spec) * vec3(0, 1, 0);
    vec3 specular = vec3(spec);

    // calculate shadow
    float shadow = ShadowCalculation(fs_in.FragPos, normal);
    vec3 lighting = (ambient + vec3(1 - shadow) * (diffuse + specular)) * color;

    FragColor = vec4(lighting, 1.0);
}
)";

inline const char* default_vs_shader = R"(
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

)";
