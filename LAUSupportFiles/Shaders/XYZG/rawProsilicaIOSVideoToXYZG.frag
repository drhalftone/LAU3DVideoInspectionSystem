#version 330 core

#define LOFREQUENCY     4.0
#define HIFREQUENCY    24.0

uniform sampler2D qt_depthTexture; // THIS TEXTURE HOLDS THE DFT COEFFICIENTS AS A 3-D TEXTURE
uniform sampler2D qt_spherTexture; // THIS TEXTURE HOLDS THE A, B, C, D, E, F, G, AND H COEFFICIENTS
uniform sampler2D qt_colorTexture; // THIS TEXTURE HOLDS THE COLOR BUFFER
uniform sampler2D qt_unwrpTexture; // THIS TEXTURE HOLDS THE UNWRAPPING THRESHOLDS

uniform     float qt_snrThreshold; // THIS HOLDS THE MAGNITUDE THRESHOLD
uniform     float qt_mtnThreshold; // THIS HOLDS THE MAGNITUDE THRESHOLD
uniform      vec2 qt_depthLimits;  // THIS HOLDS THE RANGE LIMITS OF THE LOOK UP TABLE

layout(location = 0, index = 0) out vec4 qt_fragColor;

void main()
{
    // GET THE PIXEL COORDINATE OF THE CURRENT FRAGMENT
    int row = int(gl_FragCoord.y);
    int col = int(gl_FragCoord.x);

    // GET THE LOW FREQUENCY DFT COEFFICIENTS
    vec4 vertexLo = 16.0 * texelFetch(qt_depthTexture, ivec2(2*col,row), 0);

    // DERIVE THE WRAPPED PHASE TERMS
    float phaseLo = 0.5 - atan(vertexLo.y, vertexLo.x)/6.28318530717959;
    float phaseHi = 0.5 - atan(vertexLo.w, vertexLo.z)/6.28318530717959;

    // DERIVE THE UNWRAPPED PHASE TERMS
    float phase = (round(HIFREQUENCY * phaseLo / LOFREQUENCY - phaseHi) + phaseHi) * LOFREQUENCY / HIFREQUENCY;

    // GRAB THE PHASE UNWRAPPING THRESHOLD
    float unwrp = texelFetch(qt_unwrpTexture, ivec2(col,row), 0).r;

    // UNWRAP THE PHASE VALUE
    phase = phase + float(phase < unwrp);

    // DERIVE THE MAGNITUDE TERM
    float magnitudeA = length(vertexLo.xy);
    float magnitudeB = length(vertexLo.zw);

    // USE THE FOLLOWING COMMAND TO GET FOUR FLOATS FROM THE TEXTURE BUFFER
    vec4 vecABCD = texelFetch(qt_spherTexture, ivec2(3*col+0,row), 0);
    vec4 vecEFGH = texelFetch(qt_spherTexture, ivec2(3*col+1,row), 0);
    vec4 vecIJKL = texelFetch(qt_spherTexture, ivec2(3*col+2,row), 0);

    // DERIVE THE XYZ COORDINATES FROM THE PHASE VALUE
    qt_fragColor.b  = vecEFGH.x * phase * phase * phase * phase;
    qt_fragColor.b += vecEFGH.y * phase * phase * phase;
    qt_fragColor.b += vecEFGH.z * phase * phase;
    qt_fragColor.b += vecEFGH.w * phase;
    qt_fragColor.b += vecIJKL.x;

    qt_fragColor.r = (vecABCD.x * qt_fragColor.b + vecABCD.y);
    qt_fragColor.g = (vecABCD.z * qt_fragColor.b + vecABCD.w);

    // DERIVE CUMMULATIVE FLAG TERM
    float snrFlag = float(magnitudeA > qt_snrThreshold);
    float rngFlag = float(qt_fragColor.b > qt_depthLimits.x) * float(qt_fragColor.b < qt_depthLimits.y);
    float cumFlag = snrFlag * rngFlag;

    // APPLY SNR FLAG TO EITHER PRESERVE SAMPLE OR SET TO ALL NANS
    qt_fragColor.xyz = (qt_fragColor.xyz * cumFlag) / cumFlag;

    // SET THE OUTPUT SURFACE TEXTURE TO THE K=1 DFT COEFFICIENT'S MAGNITUDE
    //qt_fragColor.x = phaseLo;
    //qt_fragColor.y = phaseHi;
    //qt_fragColor.z = phase;
    qt_fragColor.a = 0.5 * (magnitudeA + magnitudeB);
}
