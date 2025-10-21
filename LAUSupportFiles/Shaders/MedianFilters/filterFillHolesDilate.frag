#version 330 core

uniform sampler2D qt_texture;   // THIS TEXTURE HOLDS THE RGB TEXTURE COORDINATES
uniform       int qt_radius;

layout(location = 0, index = 0) out vec4 qt_fragColor;

void main()
{
    // GET THE FRAGMENT PIXEL COORDINATE
    ivec2 coord = ivec2(gl_FragCoord.xy);

    // GRAB THE CURRENT PIXEL TO PROCESS
    qt_fragColor = texelFetch(qt_texture, coord, 0);

    // MAKE A LOCAL COPY AND ITERATE THROUGH SEARCH RADIUS
    vec4 pixel = qt_fragColor;
    for (int row = -qt_radius; row <= qt_radius; row++){
        for (int col = -qt_radius; col <= qt_radius; col++){
            // KEEP THE MAXIMUM VALUE BETWEEN THE WINDOWED PIXEL AND THE PREVIOUS MAX
            pixel = max(pixel, texelFetch(qt_texture, ivec2(coord + ivec2(col, row)), 0));
        }
    }

    // LET'S AVOID AN IF STATEMENT BY CONVERTING THE LOGICAL VALUE TO FLOATING POINT
    vec4 lambda = vec4(lessThan(qt_fragColor, vec4(0.000001, 0.000001, 0.000001, 0.000001)));

    // USE THE LOGICAL VALUE AS AN INTERPOLATION BETWEEN THE MAX PIXEL IN THE WINDOW
    // AND WITH THE ORIGINAL PIXEL SO THAT WE ONLY REPLACE ZEROS
    qt_fragColor = lambda * pixel + (1.0 - lambda) * qt_fragColor;

    return;
}
