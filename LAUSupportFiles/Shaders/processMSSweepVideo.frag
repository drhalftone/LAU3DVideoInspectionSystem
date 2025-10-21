#version 330 core

uniform sampler3D qt_video;              // THIS TEXTURE HOLDS THE VIDEO SEQUENCE
uniform sampler3D qt_map;                // THIS TEXTURE HOLDS THE PIXEL-BY-PIXEL MAPPING
uniform sampler2D qt_background;         // THIS TEXTURE HOLDS THE BACKGROUND IMAGE
uniform int       qt_channel = 0;        // THIS PARAMETER TELLS US WHAT CHANNEL TO RENDER
uniform float     qt_scaleFactor = 1.0f; // SCALE VIDEO IF ITS NOT ENTIRELY 8 OR 16 BITS PER PIXEL

layout(location = 0, index = 0) out vec4 qt_fragColor;

void main()
{
    // READ THE MAP VECTOR
    vec4 map = texelFetch(qt_map, ivec3(ivec2(gl_FragCoord.xy), qt_channel), 0);

    // READ THE BACKGROUND INTENSITY
    vec4 background = texelFetch(qt_background, ivec2(map.xy), 0);

    // SET THE OUTPUT COLOR FROM THE VIDEO TEXTURE
    qt_fragColor = texelFetch(qt_video, ivec3(map.xyz), 0);

    // APPLY A TWO-POINT CORRECTION TO REMOVE THE BACKGROUND AND RESCALE THE PIXEL VALUES
    qt_fragColor.rgb = (qt_fragColor.rgb - background.rgb) * qt_scaleFactor;

    //qt_fragColor = vec4(qt_fragColor.rgb * qt_scaleFactor, 1.0);
    //qt_fragColor = vec4(background.xyz * qt_scaleFactor, 1.0);
}
