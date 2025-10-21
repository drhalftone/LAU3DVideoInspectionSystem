#version 330 core

uniform mat4      qt_colorMat; // THIS MATRIX IS THE COLOR TRANSFORM MATRIX
uniform sampler2D qt_texture;  // HOLDS THE CURRENT TEXTURE

in vec2 qt_texCoord;          // TEXTURE COORDINATE FOR RESCALING THE TEXTURE TO FIT THE SCREEN

layout(location = 0, index = 0) out vec4 qt_fragColor;

void main()
{
    // LOOKUP THE TEXTURE BY POSITION ON SCREEN
    qt_fragColor = qt_colorMat * texture(qt_texture, qt_texCoord);
}
