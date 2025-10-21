#version 330 core

uniform mat4 qt_color;
uniform int  qt_mode;
in      vec4 qt_fragment;

layout(location = 0, index = 0) out vec4 qt_fragColor;

void main()
{
    // JUST PASS THE ALBEDO FROM THE VERTEX SHADER TO THE DISPLAY
    if (qt_mode == 0){
        qt_fragColor = qt_color * vec4(qt_fragment.rgb, 1.0);
    } else if (qt_mode == 1){
        qt_fragColor = vec4(qt_fragment.rgb/2.0+0.5, 1.0);
    } else if (qt_mode == 2 || qt_mode == 12){
        qt_fragColor = qt_fragment;
    } else if (qt_mode == 3){
        qt_fragColor = vec4(1.0 - qt_fragment.zzz, 1.0);
    }
}
