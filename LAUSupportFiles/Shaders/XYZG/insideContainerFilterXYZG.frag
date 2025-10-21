#version 330 core

uniform sampler2D qt_texture;    // THIS TEXTURE HOLDS THE XYZ+TEXTURE COORDINATES
uniform sampler2D qt_background; // THIS TEXTURE HOLDS THE XYZ+TEXTURE COORDINATES
uniform      mat4 qt_projection; // THIS MATRIX IS THE PROJECTION MATRIX

uniform      vec2 qt_xLimits;    // THIS HOLDS THE RANGE LIMITS OF THE CONTAINER ALONG THE X AXIS
uniform      vec2 qt_yLimits;    // THIS HOLDS THE RANGE LIMITS OF THE CONTAINER ALONG THE Y AXIS
uniform      vec2 qt_zLimits;    // THIS HOLDS THE RANGE LIMITS OF THE CONTAINER ALONG THE Z AXIS

layout(location = 0, index = 0) out vec4 qt_fragColor;

void main()
{
    // READ THE INCOMING XYZW POINT FROM THE USER SUPPLIED TEXTURE
    vec4 point = texelFetch(qt_texture, ivec2(gl_FragCoord.x, gl_FragCoord.y), 0);

    // GET THE DEPTH THRESHOLD OF THE BACKGROUND
    float background = texelFetch(qt_background, ivec2(gl_FragCoord.x, gl_FragCoord.y), 0).r;

    // APPLY CONTAINER LIMITS AXIS BY AXIS
    bool xFlag = (qt_xLimits.x < point.x) && (point.x < qt_xLimits.y);
    bool yFlag = (qt_yLimits.x < point.y) && (point.y < qt_yLimits.y);
    bool zFlag = (qt_zLimits.x < point.z) && (point.z < qt_zLimits.y);
    bool aFlag = (point.w < background) || (background < 1e-6);

    // MERGE THE POINT COORDINATE WITH THE NEW ALPHA CHANNEL
    qt_fragColor = vec4(point.xyz, float(xFlag && yFlag && zFlag && aFlag));

    // DELETE THE POINTS OUTSIDE THE CONTAINER
    qt_fragColor.xyz = (qt_fragColor.xyz * qt_fragColor.w) / qt_fragColor.w;
    qt_fragColor = qt_fragColor.wxyz;
}
