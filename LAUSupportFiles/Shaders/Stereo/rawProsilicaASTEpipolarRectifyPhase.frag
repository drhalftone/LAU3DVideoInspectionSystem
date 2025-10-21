#version 330 core

uniform sampler2D qt_phaseTexture;   // THIS TEXTURE HOLDS THE PHASES OF THE TWO CAMERAS THAT WE ARE RECTIFYING
uniform sampler2D qt_mappingTexture; // THIS TEXTURE HOLDS THE MAPPINGS FOR THE TWO PHASE IMAGES

layout(location = 0, index = 0) out vec4 qt_fragColor;

void main()
{
    // GET THE PIXEL COORDINATE OF THE CURRENT FRAGMENT
    int row = int(gl_FragCoord.y);
    int col = int(gl_FragCoord.x);

    // GRAB THE RECTIFIED COORDINATES FOR THIS PIXEL
    vec4 position = texelFetch(qt_mappingTexture, ivec2(col, row), 0);

    // GRAB THE PHASE IMAGES USING THE MAPPING COORINATES
    vec2 pixelA = texture2D(qt_phaseTexture, position.xy, 0).xy;
    vec2 pixelB = texture2D(qt_phaseTexture, position.zw, 0).zw;

    // MERGE THE TWO PHASE AND MAGNITUDE PAIRS INTO AN EPIPOLAR RECTIFIED PIXEL
    qt_fragColor = vec4(pixelA, pixelB);

    return;
}
