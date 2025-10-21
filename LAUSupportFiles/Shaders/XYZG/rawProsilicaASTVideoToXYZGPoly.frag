#version 330 core

#define LOFREQUENCY     1.0
#define MEFREQUENCY     6.0
#define HIFREQUENCY    30.0    // 24.0

uniform sampler3D qt_depthTexture;   // THIS TEXTURE HOLDS THE DFT COEFFICIENTS AS A 3-D TEXTURE
uniform sampler2D qt_spherTexture;   // THIS TEXTURE HOLDS THE A, B, C, D, E, F, G, AND H COEFFICIENTS
uniform sampler1D qt_phaseTexture;   // THIS TEXTURE HOLDS THE PHASE CORRECTION TABLE

uniform       int qt_numCameras = 2; // THIS HOLDS THE NUMBER OF CAMERAS THAT WE NEED TO PROCESS
uniform     float qt_snrThreshold;   // THIS HOLDS THE MAGNITUDE THRESHOLD
uniform     float qt_mtnThreshold;   // THIS HOLDS THE MAGNITUDE THRESHOLD

layout(location = 0, index = 0) out vec4 qt_fragColor;

void main()
{
    // GET THE PIXEL COORDINATE OF THE CURRENT FRAGMENT
    int row = int(gl_FragCoord.y);
    int col = int(gl_FragCoord.x);

    // CREATE VECTORS OF PHASE AND MAGNITUDES FOR EACH CAMERA
    vec4 phase;
    vec4 magnitude;

    // ITERATE THROUGH THE CAMERAS
    for (int cmr = 0; cmr < qt_numCameras; cmr++){
        // GET THE LOW FREQUENCY DFT COEFFICIENTS
        vec4 vertexLo = 16.0 * texelFetch(qt_depthTexture, ivec3(3*col+0, row, cmr), 0);
        vec4 vertexMe = 16.0 * texelFetch(qt_depthTexture, ivec3(3*col+1, row, cmr), 0);
        vec4 vertexHi = 16.0 * texelFetch(qt_depthTexture, ivec3(3*col+2, row, cmr), 0);

        // DERIVE THE WRAPPED PHASE TERMS
        float phaseLo = 0.5 - atan(vertexLo.y, vertexLo.x)/6.28318530717959;
        float phaseMe = 0.5 - atan(vertexMe.y, vertexMe.x)/6.28318530717959;
        float phaseHi = 0.5 - atan(vertexHi.y, vertexHi.x)/6.28318530717959;

        // DERIVE THE UNWRAPPED PHASE TERMS
        phase[cmr] = (round(MEFREQUENCY / LOFREQUENCY * phaseLo - phaseMe) + phaseMe) / MEFREQUENCY * LOFREQUENCY;
        phase[cmr] = (round(HIFREQUENCY / LOFREQUENCY * phase[cmr] - phaseHi) + phaseHi) / HIFREQUENCY * LOFREQUENCY;

        // USE A TABLE TO PERFORM PHASE CORRECTION
        phase[cmr] = texture(qt_phaseTexture, phase[cmr]).r;

        // DERIVE THE MAGNITUDE TERM
        float magnitudeA = 0.3334 * (length(vertexLo.xy) + length(vertexMe.xy) + length(vertexHi.xy));
        float magnitudeB = 0.3334 * (length(vertexLo.zw) + length(vertexMe.zw) + length(vertexHi.zw));

        // RECORD THE MAGNITUDE FOR THIS CAMERA
        magnitude[cmr] = magnitudeA;

        // DERIVE CUMMULATIVE FLAG TERM
        float snrFlag = float(magnitudeA > qt_snrThreshold);
        float ratFlag = float((length(vertexHi.xy) / length(vertexLo.xy)) > 0.2);
        float mgnFlag = float((magnitudeB/magnitudeA) < qt_mtnThreshold);
        float cumFlag = snrFlag * mgnFlag; // * rngFlag; // * ratFlag;

        // APPLY THE FLAG TO OUR PHASE VALUE
        phase[cmr] = (phase[cmr] * cumFlag) / cumFlag * (cumFlag / cumFlag);
    }

    // ENCODE THE PHASE AND MAGNITUDES OF THE TWO CAMERAS TO THE OUTPUT PIXEL
    qt_fragColor = vec4(magnitude[0], phase[0], magnitude[1], phase[1]);

    return;
}
