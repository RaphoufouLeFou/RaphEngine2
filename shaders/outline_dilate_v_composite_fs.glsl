#version 410 core

in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D hDilatedTex; // rg: coverage, nearest object depth
uniform sampler2D originalMaskTex; // r: pre-dilation coverage
uniform sampler2D sceneDepthTex; // real, pristine scene depth
uniform int radius;
uniform vec2 texelSize;
uniform vec3 outlineColor;

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
    const float depthBias = 0.0005;

    if (sceneDepthHere < nearestDepth - depthBias)
        discard;

    FragColor = vec4(outlineColor, 1.0);
}
