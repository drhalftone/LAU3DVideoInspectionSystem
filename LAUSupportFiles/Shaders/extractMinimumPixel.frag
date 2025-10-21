#version 330 core

uniform sampler2D qt_textureA; // HOLDS THE CURRENT TEXTURE
uniform sampler2D qt_textureB; // HOLDS THE CURRENT TEXTURE

layout(location = 0, index = 0) out vec4 qt_fragColor;

void main()
{
    // CALCULATE A TEXEL COORDINATE FROM THE FRAGMENT COORDINATE
    ivec2 coord = ivec2(gl_FragCoord.xy);

    // LOOKUP THE TEXTURE BY POSITION ON SCREEN
    vec4 pixelA = texelFetch(qt_textureA, coord, 0);
    vec4 pixelB = texelFetch(qt_textureB, coord, 0);

    // REMOVE ZEROS FROM THE INCOMING VIDEO FRAME
    pixelA = pixelA + vec4(lessThan(pixelA, vec4(0.001, 0.001, 0.001, 0.001)));

    // SET THE OUTPUT TO THE MAXIMUM VALUE FROM EACH DIMENSION OF PIXELS A AND B
    qt_fragColor = min(pixelA, pixelB);
}
