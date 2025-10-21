#version 330 core

uniform sampler2D qt_depthTexture;   // THIS TEXTURE HOLDS THE DFT COEFFICIENTS AS A 3-D TEXTURE
uniform sampler2D qt_spherTexture;   // THIS TEXTURE HOLDS THE A, B, C, D, E, F, G, AND H COEFFICIENTS
uniform      vec2 qt_depthLimits;    // THIS HOLDS THE RANGE LIMITS OF THE LOOK UP TABLE

layout(location = 0, index = 0) out vec4 qt_fragColor;

void main()
{
    // GET THE PIXEL COORDINATE OF THE CURRENT FRAGMENT
    int row = int(gl_FragCoord.y);
    int col = int(gl_FragCoord.x);

    // GET THE LOW FREQUENCY DFT COEFFICIENTS
    vec4 crspdnc = texelFetch(qt_depthTexture, ivec2(col, row), 0);

    // USE THE FOLLOWING COMMAND TO GET FOUR FLOATS FROM THE TEXTURE BUFFER
    vec4 vecABCD = texelFetch(qt_spherTexture, ivec2(3*col + 0, row), 0);
    vec4 vecEFGH = texelFetch(qt_spherTexture, ivec2(3*col + 1, row), 0);
    vec4 vecIJKL = texelFetch(qt_spherTexture, ivec2(3*col + 2, row), 0);

    // DERIVE THE XYZ COORDINATES FROM THE PHASE VALUE
    qt_fragColor.z  = vecEFGH.x * crspdnc.z * crspdnc.z * crspdnc.z * crspdnc.z;
    qt_fragColor.z += vecEFGH.y * crspdnc.z * crspdnc.z * crspdnc.z;
    qt_fragColor.z += vecEFGH.z * crspdnc.z * crspdnc.z;
    qt_fragColor.z += vecEFGH.w * crspdnc.z;
    qt_fragColor.z += vecIJKL.x;

    qt_fragColor.x = (vecABCD.x * qt_fragColor.b + vecABCD.y);
    qt_fragColor.y = (vecABCD.z * qt_fragColor.b + vecABCD.w);

    // DERIVE CUMMULATIVE FLAG TERM
    float rngFlag = float(qt_fragColor.z > qt_depthLimits.x) * float(qt_fragColor.z < qt_depthLimits.y);

    // SET UNRESOLVED PIXELS TO NAN
    qt_fragColor = (qt_fragColor * rngFlag) / rngFlag * (rngFlag / rngFlag);

    // COPY OVER THE MAGNITUDE OF THE MASTER CAMERA
    //qt_fragColor.x = qt_depthLimits.x;
    //qt_fragColor.y = qt_depthLimits.y;
    //qt_fragColor.a = crspdnc.w;
    qt_fragColor.a = crspdnc.w;

    return;
}
