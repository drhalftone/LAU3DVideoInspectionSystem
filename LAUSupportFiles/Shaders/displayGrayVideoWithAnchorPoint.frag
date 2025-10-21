#version 330 core

uniform sampler2D qt_texture;   // THIS TEXTURE HOLDS THE XYZ+TEXTURE COORDINATES
uniform      vec2 qt_anchor;    // THIS HOLDS THE FRAGMENT COORDINATE OF THE ANCHOR POINT

in vec2 qt_coordinate;

layout(location = 0, index = 0) out vec4 qt_fragColor;

void main()
{
    // MAP THE FRAGMENT COORDINATES TO TEXTURE COORDINATES
    vec2 coordinate = abs(gl_FragCoord.xy - qt_anchor);

    if (max(coordinate.x, coordinate.y) < 10.0){
        qt_fragColor = vec4(1.0, 0.0, 0.0, 1.0);
    } else {
        // GET THE PIXEL COORDINATE OF THE CURRENT FRAGMENT
        qt_fragColor = texture(qt_texture, qt_coordinate, 0).rrra;
        if (qt_fragColor.r > 0.0){
            qt_fragColor = 1.0 - sqrt(qt_fragColor);
        }
    }
}
