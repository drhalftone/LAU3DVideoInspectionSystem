#version 330 core

uniform      bool qt_flip = false;
uniform sampler2D qt_texture;            // THIS TEXTURE HOLDS THE XYZ+TEXTURE COORDINATES
in           vec2 qt_coordinate;         // HOLDS THE TEXTURE COORDINATE FROM THE VERTEX SHADER

layout(location = 0, index = 0) out vec4 qt_fragColor;

void main()
{
    // GET THE PIXEL COORDINATE OF THE CURRENT FRAGMENT
    if (qt_flip){
        qt_fragColor = vec4(texture(qt_texture, qt_coordinate, 0).rrr, 1.0);
    } else {
        qt_fragColor = vec4(texture(qt_texture, vec2(1.0 - qt_coordinate.x, 1.0 - qt_coordinate.y), 0).rrr, 1.0);
    }
}
