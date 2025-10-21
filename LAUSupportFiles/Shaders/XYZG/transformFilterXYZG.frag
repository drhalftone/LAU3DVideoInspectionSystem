#version 330 core

uniform sampler2D qt_texture;       // THIS TEXTURE HOLDS THE XYZ+TEXTURE COORDINATES
uniform      mat4 qt_projection;    // THIS MATRIX IS THE PROJECTION MATRIX

layout(location = 0, index = 0) out vec4 qt_fragColor;

void main()
{
    // NOW USE THE STANDARD DEVIATION TO CHANGE THE LUMINANCE OF THE INCOMING COLOR
    vec4 point = texelFetch(qt_texture, ivec2(gl_FragCoord.x, gl_FragCoord.y), 0);
    qt_fragColor = vec4(point.xyz, 1.0);
    qt_fragColor = qt_projection * qt_fragColor;
    qt_fragColor.a = point.a;
}
