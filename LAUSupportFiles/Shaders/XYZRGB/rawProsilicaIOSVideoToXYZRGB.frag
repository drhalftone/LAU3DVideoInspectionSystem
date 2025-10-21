#version 330 core

#define LOFREQUENCY     8.0
#define HIFREQUENCY    32.0

uniform sampler2D qt_depthTexture; // THIS TEXTURE HOLDS THE DFT COEFFICIENTS AS A 3-D TEXTURE
uniform sampler2D qt_spherTexture; // THIS TEXTURE HOLDS THE A, B, C, D, E, F, G, AND H COEFFICIENTS
uniform sampler2D qt_colorTexture; // THIS TEXTURE HOLDS THE COLOR BUFFER
uniform sampler2D qt_unwrpTexture; // THIS TEXTURE HOLDS THE PHASE THRESHOLD FOR UNWRAPPING

uniform     float qt_snrThreshold; // THIS HOLDS THE MAGNITUDE THRESHOLD
uniform     float qt_mtnThreshold; // THIS HOLDS THE MAGNITUDE THRESHOLD
uniform      vec2 qt_depthLimits;  // THIS HOLDS THE RANGE LIMITS OF THE LOOK UP TABLE

layout(location = 0, index = 0) out vec4 qt_fragColor;

void main()
{
    // GET THE ROW OFFSET TO HANDLE BAYER FILTER PATTERNS
    int rowOffset = int(gl_FragCoord.x + gl_FragCoord.y)%2;

    // GET THE PIXEL COORDINATE OF THE CURRENT FRAGMENT
    if (int(gl_FragCoord.x)%2 == 0){
        // GET THE PIXEL COORDINATE OF THE CURRENT FRAGMENT
        int row = int(gl_FragCoord.y) - rowOffset;
        int col = int(gl_FragCoord.x / 2);

        // GET THE LOW FREQUENCY DFT COEFFICIENTS
        vec4 vertexLo = texelFetch(qt_depthTexture, ivec2(2 * col,row), 0);

        // UNCLAMP THE DFT COEFFICIENTS AND RESCALE THEM
        vertexLo = 8.0 * (vertexLo - vec4(float(vertexLo.x >= 0.5), float(vertexLo.y >= 0.5), float(vertexLo.z >= 0.5), float(vertexLo.w >= 0.5)));

        // DERIVE THE WRAPPED PHASE TERMS
        float phaseLo = 0.5 - atan(vertexLo.y, vertexLo.x)/6.28318530717959;
        float phaseHi = 0.5 - atan(vertexLo.w, vertexLo.z)/6.28318530717959;

        // DERIVE THE UNWRAPPED PHASE TERMS
        float phase = (round(HIFREQUENCY * phaseLo / LOFREQUENCY - phaseHi) + phaseHi) * LOFREQUENCY / HIFREQUENCY;

        // GRAB THE PHASE UNWRAPPING THRESHOLD
        float unwrp = texelFetch(qt_unwrpTexture, ivec2(col,row), 0).r;

        // UNWRAP THE PHASE VALUE AND AMPLIFY BY 10 FOR THE POLYNOMIAL
        phase = 10.0 * (phase + float(phase < unwrp));

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
        float mgnFlag = float((magnitudeB/magnitudeA) < qt_mtnThreshold);
        float rngFlag = float(qt_fragColor.b > qt_depthLimits.x) * float(qt_fragColor.b < qt_depthLimits.y);
        float cumFlag = snrFlag * mgnFlag * rngFlag;

        // APPLY SNR FLAG TO EITHER PRESERVE SAMPLE OR SET TO ALL NANS
        qt_fragColor = vec4((qt_fragColor.xyz * cumFlag)/cumFlag * (cumFlag/cumFlag), cumFlag);

        // SET THE OUTPUT SURFACE TEXTURE TO THE K=1 DFT COEFFICIENT'S MAGNITUDE
        //qt_fragColor.x = phaseLo;
        //qt_fragColor.y = phaseHi;
        //qt_fragColor.z = phase;
        //qt_fragColor.a = qt_snrThreshold;
    } else {
        // PASS THROUGH THE RGB TEXTURE
        qt_fragColor = texelFetch(qt_colorTexture, ivec2(gl_FragCoord.x/2, gl_FragCoord.y), 0) * vec4(1.2580, 1.0000, 1.1249, 1.0000);
    }
}
