#version 330 core

#define LOFREQUENCY     1.0
#define MEFREQUENCY     6.0
#define HIFREQUENCY    36.0

uniform sampler2D qt_depthTexture;   // THIS TEXTURE HOLDS THE DFT COEFFICIENTS AS A 3-D TEXTURE
uniform sampler2D qt_spherTexture;   // THIS TEXTURE HOLDS THE A, B, C, D, E, F, G, AND H COEFFICIENTS
uniform sampler2D qt_mappingTexture; // THIS TEXTURE HOLDS THE UNWRAPPED PHASE DERIVED FROM TOF DEPTH

uniform     float qt_snrThreshold;   // THIS HOLDS THE MAGNITUDE THRESHOLD
uniform     float qt_mtnThreshold;   // THIS HOLDS THE MAGNITUDE THRESHOLD
uniform      vec2 qt_depthLimits;    // THIS HOLDS THE RANGE LIMITS OF THE LOOK UP TABLE

layout(location = 0, index = 0) out vec4 qt_fragColor;

void main()
{
    // GET THE PIXEL COORDINATE OF THE CURRENT FRAGMENT
    int row = int(gl_FragCoord.y);
    int col = int(gl_FragCoord.x);

    // GET THE LOW FREQUENCY DFT COEFFICIENTS
    vec4 vertexLo = 16.0 * texelFetch(qt_depthTexture, ivec2(3*col+0,row), 0);
    vec4 vertexMe = 16.0 * texelFetch(qt_depthTexture, ivec2(3*col+1,row), 0);
    vec4 vertexHi = 16.0 * texelFetch(qt_depthTexture, ivec2(3*col+2,row), 0);

    // DERIVE THE WRAPPED PHASE TERMS
    // float phaseLo = 0.5 - atan(vertexLo.y, vertexLo.x)/6.28318530717959;
    // float phaseMe = 0.5 - atan(vertexMe.y, vertexMe.x)/6.28318530717959;
    float phaseHi = 0.5 - atan(vertexHi.y, vertexHi.x)/6.28318530717959;

    // DERIVE THE UNWRAPPED PHASE TERMS
    // float phase = (round(MEFREQUENCY/LOFREQUENCY*phaseLo - phaseMe) + phaseMe) / MEFREQUENCY * LOFREQUENCY;

    // READ THE LOW FREQUENCY PHASE IMAGE FROM THE MAPPING BUFFER
    float phase = texelFetch(qt_mappingTexture, ivec2(col/4, row), 0)[col%4];
          phase = (round(HIFREQUENCY/LOFREQUENCY*phase   - phaseHi) + phaseHi) / HIFREQUENCY * LOFREQUENCY;

    // DERIVE THE MAGNITUDE TERM
    float magnitudeA = 0.3334 * (length(vertexLo.xy) + length(vertexMe.xy) + length(vertexHi.xy));
    float magnitudeB = 0.3334 * (length(vertexLo.zw) + length(vertexMe.zw) + length(vertexHi.zw));

    // USE THE FOLLOWING COMMAND TO GET FOUR FLOATS FROM THE TEXTURE BUFFER
    vec4 vecABCD = texelFetch(qt_spherTexture, ivec2(2*col+0,row), 0);
    vec4 vecEFGH = texelFetch(qt_spherTexture, ivec2(2*col+1,row), 0);

    // DERIVE THE XYZ COORDINATES FROM THE PHASE VALUE
    qt_fragColor.z = (vecEFGH.x * phase + vecEFGH.y)/(vecEFGH.z * phase + vecEFGH.w);
    qt_fragColor.x = (vecABCD.x * qt_fragColor.b + vecABCD.y);
    qt_fragColor.y = (vecABCD.z * qt_fragColor.b + vecABCD.w);

    // DERIVE CUMMULATIVE FLAG TERM
    float snrFlag = float(magnitudeA > qt_snrThreshold);
    float mgnFlag = float((magnitudeB/magnitudeA) < qt_mtnThreshold);
    float rngFlag = float(qt_fragColor.b > qt_depthLimits.x) * float(qt_fragColor.b < qt_depthLimits.y);
    float cumFlag = snrFlag * mgnFlag * rngFlag;

    // APPLY SNR FLAG TO EITHER PRESERVE SAMPLE OR SET TO ALL NANS
    qt_fragColor.xyz = (qt_fragColor.xyz * cumFlag) / cumFlag;

    // SET THE OUTPUT SURFACE TEXTURE TO THE K=1 DFT COEFFICIENT'S MAGNITUDE
    qt_fragColor.a = 2.0 * magnitudeA;
}
