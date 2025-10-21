#version 330 core

in vec2 qt_fragment;      // HOLDS THE UNDISTORTED PIXEL COORDINATE FOR THE FRAGMENT SHADER

layout(location = 0, index = 0) out vec4 qt_fragColor;

void main()
{
    qt_fragColor = vec4(qt_fragment.xy, 0.0, 1.0);
}
