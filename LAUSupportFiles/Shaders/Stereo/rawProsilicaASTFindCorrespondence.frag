#version 330 core

uniform sampler2D qt_phaseTexture;   // THIS TEXTURE HOLDS THE PHASES OF THE TWO CAMERAS THAT WE ARE RECTIFYING
uniform sampler2D qt_mappingTexture; // THIS TEXTURE HOLDS THE MAPPINGS FOR THE TWO PHASE IMAGES

layout(location = 0, index = 0) out vec4 qt_fragColor;

void main()
{
    // GET THE PIXEL COORDINATE OF THE CURRENT FRAGMENT
    int col = int(gl_FragCoord.x);
    int row = int(gl_FragCoord.y);

    // INITIALIZE THE OUTPUT PIXEL
    qt_fragColor = vec4(-1.0, -1.0, -1.0, -1.0);

    // GRAB THE PHASE OF THE MASTER CAMERA USING THE FRAGMENT COORDINATE
    vec2 phaseA = texelFetch(qt_phaseTexture, ivec2(col, row), 0).xy;

    // GRAB THE MIN AND MAX COORDINATES INTO THE SLAVE CAMERA
    vec4 rangeLimits = texelFetch(qt_mappingTexture, ivec2(col, row), 0);

    // ITERATURE FROM THE MIN POSITION TO THE MAX POSITION IN LARGE STEPS
    float dOpt = 0.01;
    for (int l = 0; l < 100; l++){
        // CALCULATE THE FRACTION OF DISTANCE FROM SOURCE TO DESTINATION
        float lambda = float(l) / 100.0;

        // CALCULATE A POSITION ALONG THE LINE OF SIGHT
        vec2 position = lambda * rangeLimits.zw + (1.0 - lambda) * rangeLimits.xy;

        // GRAB THE PHASE OF THE SLAVE CAMERA ALONG THE LINE OF SIGHT
        vec2 phaseB = texture2D(qt_phaseTexture, position, 0).zw;

        // COMPARE THIS PHASE FROM THE SLAVE CAMERA TO THE PHASE OF THE MASTER
        float d = abs(phaseA.y - phaseB.y);
        if (d < dOpt){
            // KEEP TRACK OF THE OPTIMAL DISPARITY SO FAR AND RECORD
            // BOTH PHASES FOR THIS MASTER AND THE BEST MATCHING SLAVE
            dOpt = d;
            qt_fragColor.z = lambda;
            qt_fragColor.w = phaseB.x;
        }
    }

    // ITERATURE FROM THE MIN POSITION TO THE MAX POSITION IN SMALL STEPS
    float offset = qt_fragColor.z;
    for (int l = 0; l < 400; l++){
        // CALCULATE THE FRACTION OF DISTANCE FROM SOURCE TO DESTINATION
        float lambda = float(l - 200) / 20000.0 + offset;

        // CALCULATE A POSITION ALONG THE LINE OF SIGHT
        vec2 position = lambda * rangeLimits.zw + (1.0 - lambda) * rangeLimits.xy;

        // GRAB THE PHASE OF THE SLAVE CAMERA ALONG THE LINE OF SIGHT
        vec2 phaseB = texture2D(qt_phaseTexture, position, 0).zw;

        // COMPARE THIS PHASE FROM THE SLAVE CAMERA TO THE PHASE OF THE MASTER
        float d = abs(phaseA.y - phaseB.y);
        if (d <= dOpt){
            // KEEP TRACK OF THE OPTIMAL DISPARITY SO FAR AND RECORD
            // BOTH PHASES FOR THIS MASTER AND THE BEST MATCHING SLAVE
            dOpt = d;
            qt_fragColor.z = lambda;
            qt_fragColor.w = phaseB.x;
        }
    }

    // PASS THE MAGNITUDE AND PHASE OF THE MASTER PIXEL TO THE USER
    qt_fragColor.xy = phaseA;

    // MAKE SURE OUR PHASE DIFFERENCE IS WITHIN TOLERANCE
    float rngFlag = float(dOpt < 0.001);

    // SET UNRESOLVED PIXELS TO NAN
    qt_fragColor.zw = (qt_fragColor.zw * rngFlag) / rngFlag * (rngFlag / rngFlag);

    return;
}
