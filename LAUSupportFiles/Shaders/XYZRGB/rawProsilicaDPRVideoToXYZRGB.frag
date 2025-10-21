#version 330 core

#define LOFREQUENCY     1.0
#define MEFREQUENCY     6.0
#define HIFREQUENCY    18.0 //30.0

uniform sampler2D qt_depthTexture; // THIS TEXTURE HOLDS THE DFT COEFFICIENTS AS A 3-D TEXTURE
uniform sampler2D qt_spherTexture; // THIS TEXTURE HOLDS THE A, B, C, D, E, F, G, AND H COEFFICIENTS
uniform sampler2D qt_colorTexture; // THIS TEXTURE HOLDS THE COLOR BUFFER

uniform     float qt_snrThreshold; // THIS HOLDS THE MAGNITUDE THRESHOLD
uniform     float qt_mtnThreshold; // THIS HOLDS THE MAGNITUDE THRESHOLD
uniform      vec2 qt_depthLimits;  // THIS HOLDS THE RANGE LIMITS OF THE LOOK UP TABLE

layout(location = 0, index = 0) out vec4 qt_fragColor;

void main()
{
    // GET THE PIXEL COORDINATE OF THE CURRENT FRAGMENT
    if (int(gl_FragCoord.x)%2 == 0){
        // GET THE PIXEL COORDINATE OF THE CURRENT FRAGMENT
        int row = int(gl_FragCoord.y);
        int col = int(gl_FragCoord.x);

        // GET THE LOW FREQUENCY DFT COEFFICIENTS
        vec4 vertexLo = texelFetch(qt_depthTexture, ivec2(3*col+0,row), 0);
        vec4 vertexMe = texelFetch(qt_depthTexture, ivec2(3*col+1,row), 0);
        vec4 vertexHi = texelFetch(qt_depthTexture, ivec2(3*col+2,row), 0);

        // UNCLAMP THE DFT COEFFICIENTS AND RESCALE THEM
        vertexLo = 8.0 * (vertexLo - vec4(float(vertexLo.x >= 0.5), float(vertexLo.y >= 0.5), float(vertexLo.z >= 0.5), float(vertexLo.w >= 0.5)));
        vertexMe = 8.0 * (vertexMe - vec4(float(vertexMe.x >= 0.5), float(vertexMe.y >= 0.5), float(vertexMe.z >= 0.5), float(vertexMe.w >= 0.5)));
        vertexHi = 8.0 * (vertexHi - vec4(float(vertexHi.x >= 0.5), float(vertexHi.y >= 0.5), float(vertexHi.z >= 0.5), float(vertexHi.w >= 0.5)));

        // DERIVE THE WRAPPED PHASE TERMS
        float phaseLo = 0.5 - atan(vertexLo.y, vertexLo.x)/6.28318530717959;
        float phaseMe = 0.5 - atan(vertexMe.y, vertexMe.x)/6.28318530717959;
        float phaseHi = 0.5 - atan(vertexHi.y, vertexHi.x)/6.28318530717959;

        // DERIVE THE UNWRAPPED PHASE TERMS
        float phase = (round( 8.0*phaseLo - phaseMe) + phaseMe) * 0.12500;
              phase = (round(32.0*phase   - phaseHi) + phaseHi) * 0.03125;

        // AMPLIFY THE PHASE TERM FOR THE CONVERSION TO Z
        phase = 10.0 * phase;

        // DERIVE THE MAGNITUDE TERM
        float magnitudeA = 0.3334 * (length(vertexLo.xy) + length(vertexMe.xy) + length(vertexHi.xy));
        float magnitudeB = 0.3334 * (length(vertexLo.zw) + length(vertexMe.zw) + length(vertexHi.zw));

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

        qt_fragColor.g = (vecABCD.z * qt_fragColor.b + vecABCD.w);
        qt_fragColor.r = (vecABCD.x * qt_fragColor.b + vecABCD.y);

        // DERIVE CUMMULATIVE FLAG TERM
        float snrFlag = float(magnitudeA > qt_snrThreshold);
        float mgnFlag = float((magnitudeB/magnitudeA) < qt_mtnThreshold);
        float rngFlag = float(qt_fragColor.b > qt_depthLimits.x) * float(qt_fragColor.b < qt_depthLimits.y);
        float cumFlag = snrFlag * mgnFlag * rngFlag;

        // APPLY SNR FLAG TO EITHER PRESERVE SAMPLE OR SET TO ALL NANS
        qt_fragColor.xyz = (qt_fragColor.xyz * cumFlag)/cumFlag;

        // SET THE ALPHA CHANNEL TO A LOGICAL 0 OR 1 DEPENDING ON THE VALIDITY OF THE SAMPLE
        qt_fragColor.a = cumFlag;
    } else {
        // PASS THROUGH THE RGB TEXTURE
        qt_fragColor = texelFetch(qt_colorTexture, ivec2(gl_FragCoord.x/2, gl_FragCoord.y), 0);
    }
}
