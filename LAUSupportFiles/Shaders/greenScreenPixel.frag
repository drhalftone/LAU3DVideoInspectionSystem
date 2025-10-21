#version 330 core

uniform sampler2D qt_textureA;         // HOLDS THE CURRENT TEXTURE
uniform sampler2D qt_textureB;         // HOLDS THE CURRENT TEXTURE
uniform     float qt_threshold = 0.05; // SCALAR CONSTANT SETS HOW FAR FROM THE GREEN SCREEN TO APPLY THRESHOLD

layout(location = 0, index = 0) out vec4 qt_fragColor;

void main()
{
    // CALCULATE A TEXEL COORDINATE FROM THE FRAGMENT COORDINATE
    ivec2 coord = ivec2(gl_FragCoord.xy);

    // LOOKUP THE TEXTURE BY POSITION ON SCREEN
    vec4 pixelA = texelFetch(qt_textureA, coord, 0);
    vec4 pixelB = texelFetch(qt_textureB, coord, 0);

    // CALCULATE THE DIFFERENCE BETWEEN THE FORE AND BACKGROUND IMAGES
    vec4 delta = pixelB - pixelA;

    // SET THE OUTPUT TO THE MAXIMUM VALUE FROM EACH DIMENSION OF PIXELS A AND B
    qt_fragColor = pixelA * vec4(greaterThan(delta, qt_threshold * pixelB));
}
