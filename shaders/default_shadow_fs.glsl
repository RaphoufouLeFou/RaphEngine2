#version 410 core

in vec2 TexCoords;

uniform bool alphaMask;
uniform float alphaCutoff;
uniform bool HaveTexture;
uniform bool HaveOpacityMap;
uniform sampler2D texture_diffuse;
uniform sampler2D texture_opacity;

void main()
{
    if (alphaMask)
    {
        float alpha = HaveOpacityMap
            ? texture(texture_opacity, TexCoords).r
            : (HaveTexture ? texture(texture_diffuse, TexCoords).a : 1.0);

        if (alpha < alphaCutoff)
            discard;
    }
}
