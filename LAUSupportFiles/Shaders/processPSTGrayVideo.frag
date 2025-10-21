#version 330 core

uniform sampler2D qt_texture;                      // THIS TEXTURE HOLDS THE RGB TEXTURE COORDINATES
uniform vec2      qt_scaleFactor = vec2(1.0, 1.0); // SCALE VIDEO IF ITS NOT ENTIRELY 8 OR 16 BITS PER PIXEL

layout(location = 0, index = 0) out vec4 qt_fragColor;

void main()
{
    vec2 sze = vec2(textureSize(qt_texture, 0));

    // GET THE PIXEL COORDINATE OF THE CURRENT FRAGMENT
    if (gl_FragCoord.y > (sze.y/2.0)){
        qt_fragColor = vec4(texelFetch(qt_texture, ivec2(gl_FragCoord.xy), 0).rrr * qt_scaleFactor.y, 1.0);
    } else {
        qt_fragColor = vec4(texelFetch(qt_texture, ivec2(gl_FragCoord.xy), 0).rrr * qt_scaleFactor.x, 1.0);
    }

}
