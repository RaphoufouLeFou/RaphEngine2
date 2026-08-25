#version 410 core

in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D hDilatedTex;
uniform sampler2D originalMaskTex;
uniform sampler2D sceneDepthTex;
uniform int radius;
uniform vec2 texelSize;
uniform vec3 outlineColor;
uniform float nearPlane;
uniform float farPlane;

float LinearizeDepth(float depth)
{
    float z = depth * 2.0 - 1.0;
    return (2.0 * nearPlane * farPlane)
        / (farPlane + nearPlane - z * (farPlane - nearPlane));
}

void main()
{
    float coverage = 0.0;
    float nearestDepth = 1.0;
    for (int i = -radius; i <= radius; i++)
    {
        vec2 uv = TexCoord + vec2(0.0, texelSize.y * float(i));
        vec2 s = texture(hDilatedTex, uv).rg;
        if (s.r > 0.5)
        {
            coverage = 1.0;
            nearestDepth = min(nearestDepth, s.g);
        }
    }

    float original = texture(originalMaskTex, TexCoord).r;
    if (coverage < 0.5 || original > 0.5)
        discard;

    float sceneDepthHere = texture(sceneDepthTex, TexCoord).r;

    float linearScene = LinearizeDepth(sceneDepthHere);
    float linearOutline = LinearizeDepth(nearestDepth);

    const float depthBiasWorldUnits = 0.05;
    if (linearScene < linearOutline - depthBiasWorldUnits)
        discard;

    FragColor = vec4(outlineColor, 1.0);
}
