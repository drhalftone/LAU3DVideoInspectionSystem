#version 330 core

uniform sampler2D qt_texture;            // THIS TEXTURE HOLDS THE RGB TEXTURE COORDINATES
uniform float     qt_scaleFactor = 1.0f; // SCALE VIDEO IF ITS NOT ENTIRELY 8 OR 16 BITS PER PIXEL

layout(location = 0, index = 0) out vec4 qt_fragColor;

void main()
{
    // PASS THROUGH THE RGB TEXTURE
    qt_fragColor = texelFetch(qt_texture, ivec2(gl_FragCoord.xy), 0);
    qt_fragColor.rgb = qt_fragColor.rgb * qt_scaleFactor;
}
