#version 330 core

#define LOFREQUENCY     1.0
#define MEFREQUENCY     6.0
#define HIFREQUENCY    30.0

uniform sampler2D qt_depthTexture; // THIS TEXTURE HOLDS THE DFT COEFFICIENTS AS A 3-D TEXTURE

uniform sampler2D qt_minTexture;   // THIS TEXTURE HOLDS THE MINIMUM XYZP TEXTURE
uniform sampler2D qt_maxTexture;   // THIS TEXTURE HOLDS THE MAXIMUM XYZP TEXTURE
uniform sampler3D qt_lutTexture;   // THIS TEXTURE HOLDS THE LOOK UP TABLE XYZP TEXTURE
uniform     float qt_layers;       // THIS HOLDS THE NUMBER OF LAYERS IN THE LOOK UP TABLE

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
//    vec4 vertexLo = 16.0 * texelFetch(qt_depthTexture, ivec2(3*col + 0,row), 0).zwxy;
//    vec4 vertexMe = 16.0 * texelFetch(qt_depthTexture, ivec2(3*col + 1,row), 0).zwxy;
//    vec4 vertexHi = 16.0 * texelFetch(qt_depthTexture, ivec2(3*col + 2,row), 0).zwxy;

    vec4 vertexLo = 16.0 * texelFetch(qt_depthTexture, ivec2(3*col + 0,row), 0).xyzw;
    vec4 vertexMe = 16.0 * texelFetch(qt_depthTexture, ivec2(3*col + 1,row), 0).xyzw;
    vec4 vertexHi = 16.0 * texelFetch(qt_depthTexture, ivec2(3*col + 2,row), 0).xyzw;

    // DERIVE THE WRAPPED PHASE TERMS
    float phaseLo = 0.5 - atan(vertexLo.y, vertexLo.x)/6.28318530717959;
    float phaseMe = 0.5 - atan(vertexMe.y, vertexMe.x)/6.28318530717959;
    float phaseHi = 0.5 - atan(vertexHi.y, vertexHi.x)/6.28318530717959;

    // DERIVE THE UNWRAPPED PHASE TERMS
    float phaseA = (round(MEFREQUENCY/LOFREQUENCY*phaseLo - phaseMe) + phaseMe) / MEFREQUENCY * LOFREQUENCY;
          phaseA = (round(HIFREQUENCY/LOFREQUENCY*phaseA  - phaseHi) + phaseHi) / HIFREQUENCY * LOFREQUENCY;

    // DERIVE THE MAGNITUDE TERM
    float magnitudeA = 0.3334 * (length(vertexLo.xy) + length(vertexMe.xy) + length(vertexHi.xy));
    float magnitudeB = 0.3334 * (length(vertexLo.zw) + length(vertexMe.zw) + length(vertexHi.zw));

    // USE THE FOLLOWING COMMAND TO GET FOUR FLOATS FROM THE TEXTURE BUFFER
    float minPhase = texelFetch(qt_minTexture, ivec2(col,row), 0).a;
    float maxPhase = texelFetch(qt_maxTexture, ivec2(col,row), 0).a;

    // RESCALE THE PHASE TO BE IN THE SUPPLIED RANGE
    float phaseB = (phaseA - minPhase)/(maxPhase - minPhase);

    // EXTRACT THE XYZ COORDINATE FROM THE LOOKUP TABLE
    float lambda = qt_layers * phaseB - floor(qt_layers * phaseB);
    vec4 pixelA = texelFetch(qt_lutTexture, ivec3(col,row,int(floor(qt_layers * phaseB))), 0);
    vec4 pixelB = texelFetch(qt_lutTexture, ivec3(col,row,int( ceil(qt_layers * phaseB))), 0);

    // LINEARLY INTERPOLATE BETWEEN THE TWO LAYERS EXTRACTED FROM THE LOOK UP TABLE
    qt_fragColor = pixelA + lambda * (pixelB - pixelA);

    // DERIVE CUMMULATIVE FLAG TERM
    float snrFlag = float(magnitudeA > qt_snrThreshold);
    float ratFlag = float((length(vertexHi.xy) / length(vertexLo.xy)) > 0.2);
    float rngFlag = float((qt_fragColor.b > qt_depthLimits.x) && (qt_fragColor.b < qt_depthLimits.y));
    float cumFlag = snrFlag * rngFlag * ratFlag;

    // APPLY SNR FLAG TO EITHER PRESERVE SAMPLE OR SET TO ALL NANS
    qt_fragColor.xyz = (qt_fragColor.xyz * cumFlag) / cumFlag * (cumFlag / cumFlag);

    // SET THE OUTPUT SURFACE TEXTURE TO THE K=1 DFT COEFFICIENT'S MAGNITUDE
    qt_fragColor.a = phaseA; //4.0 * magnitudeA;
}
