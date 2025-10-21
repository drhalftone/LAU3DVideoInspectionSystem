#version 330 core

uniform sampler2D qt_texture;    // THIS TEXTURE HOLDS THE XYZ TEXTURE COORDINATES
uniform sampler2D qt_valid;      // TELL US WHICH POINTS ARE INSIDE OUR CONTAINER
uniform sampler2D qt_mapping;    // THIS TEXTURE HOLDS THE MAPPING TEXTURE COORDINATES
uniform sampler2D qt_masking;    // THIS TEXTURE HOLDS THE MASK IMAGE

uniform      mat4 qt_projection; // THIS MATRIX IS THE PROJECTION MATRIX

uniform     float qt_fx;
uniform     float qt_cx;
uniform     float qt_fy;
uniform     float qt_cy;

layout(location = 0, index = 0) out vec4 qt_fragColor;

void main()
{
    // READ THE XYZ COORDINATE FROM THE SCAN TEXTURE
    vec4 point = vec4(texelFetch(qt_texture, ivec2(gl_FragCoord.xy), 0).xyz, texelFetch(qt_valid, ivec2(gl_FragCoord.xy), 0).r);

    // PROJECT THE POINT FROM ABSOLUTE WORLD TO CAMERA WORLD COORDINATES
    point = qt_projection * point;
    point = point / point.w;

    // PROJECT THE POINT FROM CAMERA WORLD TO UNDISTORTED CAMERA COORDINATES
    qt_fragColor.x = (qt_fx * point.x + qt_cx * point.z) / point.z / textureSize(qt_texture, 0).x;
    qt_fragColor.y = (qt_fy * point.y + qt_cy * point.z) / point.z / textureSize(qt_texture, 0).y;
    qt_fragColor.z = 0.0;
    qt_fragColor.w = 1.0;

    // USE THE MAPPING TEXTURE TO CONVERT FROM UNDISTORTED TO DISTORTED CAMERA COORDINATES
    qt_fragColor.zw = texture(qt_mapping, qt_fragColor.xy/2.0 + 0.25, 0).xy;

    // READ THE PIXEL FROM THE MASK TEXTURE
    qt_fragColor.x = texture(qt_masking, qt_fragColor.zw, 0).x;
    qt_fragColor.y = texelFetch(qt_masking, ivec2(gl_FragCoord.xy), 0).x;
}
