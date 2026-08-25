#version 410 core

in vec2 TexCoord;
out vec2 FragColor;

uniform sampler2D maskTex;
uniform sampler2D depthTex;
uniform int radius;
uniform vec2 texelSize;

void main()
{
    float coverage = 0.0;
    float nearestDepth = 1.0;
    for (int i = -radius; i <= radius; i++)
    {
        vec2 uv = TexCoord + vec2(texelSize.x * float(i), 0.0);
        float c = texture(maskTex, uv).r;
        if (c > 0.5)
        {
            coverage = 1.0;
            nearestDepth = min(nearestDepth, texture(depthTex, uv).r);
        }
    }
    FragColor = vec2(coverage, nearestDepth);
}
