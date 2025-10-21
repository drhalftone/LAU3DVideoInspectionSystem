#version 330 core

uniform sampler2D qt_textureA;   // HOLDS THE CURRENT FRAME TEXTURE
uniform sampler2D qt_textureB;   // HOLDS THE PREVIOUS FRAME TEXTURE
uniform       int qt_width;      // KEEPS TRACK OF THE WINDOW WIDTH

layout(location = 0, index = 0) out vec4 qt_fragColor;

void main()
{
    // KEEP TRACK OF THE CURRENT PIXELS FRAGMENT COORDINATE
    ivec2 coordB = ivec2(gl_FragCoord.xy);
    ivec2 coordA = coordB/qt_width + qt_width/2;

    // KEEP TRACK OF THE TOTAL ERROR FOR THE CURRENT WINDOW POSITION
    vec4 error = 0.0;

    // SEARCH FOR THE BEST OFFSET WITHIN A 16X16 PIXEL WINDOW SURROUNDING CURRENT PIXEL
    for (int dx = -qt_width/2; dx <= qt_width/2; dx++){
        for (int dy = -qt_width/2; dy <= qt_width/2; dy++){
            // GET THE CURRENT PIXELS FROM THEIR TEXTURES
            vec4 pixelA = texelFetch(qt_textureA, coordA + ivec2(dx, dy), 0);
            vec4 pixelB = texelFetch(qt_textureB, coordB + ivec2(dx, dy), 0);

            // UPDATE THE ERROR BASED ON THE DIFFERENCES IN COLOR
            error += abs(pixelA - pixelB);
        }
    }

    // SET OUTPUT TO BEST OFFSET AND ITS CORRESPONDING ERROR
    qt_fragColor = vec4(vec2(coordB - coordA), error, 1.0);
}
