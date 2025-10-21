#version 330 core

uniform sampler2D qt_texture;   // HOLDS THE CURRENT TEXTURE
uniform     float qt_threshold; // HOLDS THE DEPTH THRESHOLD FOR DETERMINING CONNECTEDNESS

layout(location = 0, index = 0) out vec4 qt_fragColor;

void main()
{
    // CALCULATE A TEXEL COORDINATE FROM THE FRAGMENT COORDINATE
    ivec2 coord = ivec2(gl_FragCoord.xy);

    // LOOKUP THE TEXTURE BY POSITION ON SCREEN
    vec4 pixelO = texelFetch(qt_texture, coord, 0);

    // GRAB THE 4-POINT NEIGHBORHOOD PIXELS
    vec4 pixelA = vec4(texelFetch(qt_texture, coord - ivec2(1,0), 0).a, pixelO.rgb);
    vec4 pixelB = vec4(pixelO.gba, texelFetch(qt_texture, coord + ivec2(1,0), 0).r);
    vec4 pixelC = texelFetch(qt_texture, coord - ivec2(0,1), 0);
    vec4 pixelD = texelFetch(qt_texture, coord + ivec2(0,1), 0);

    // CALCULATE THE DIFFERENCE BETWEEN THE FORE AND BACKGROUND IMAGES
    pixelA = vec4(lessThan(abs(pixelA - pixelO), vec4(qt_threshold, qt_threshold, qt_threshold, qt_threshold)));
    pixelB = vec4(lessThan(abs(pixelB - pixelO), vec4(qt_threshold, qt_threshold, qt_threshold, qt_threshold)));
    pixelC = vec4(lessThan(abs(pixelC - pixelO), vec4(qt_threshold, qt_threshold, qt_threshold, qt_threshold)));
    pixelD = vec4(lessThan(abs(pixelD - pixelO), vec4(qt_threshold, qt_threshold, qt_threshold, qt_threshold)));

    // SET THE OUTPUT TO THE MAXIMUM VALUE FROM EACH DIMENSION OF PIXELS A AND B
    qt_fragColor = pixelB/255.0 + 2.0*pixelD/255.0 + 4.0*pixelA/255.0 + 8.0*pixelC/255.0;

    // MASK ANY PIXELS THAT ARE UNDEFINED IN THE INPUT PIXEL
    qt_fragColor = qt_fragColor * vec4(greaterThan(pixelO, vec4(qt_threshold, qt_threshold, qt_threshold, qt_threshold)));
}
