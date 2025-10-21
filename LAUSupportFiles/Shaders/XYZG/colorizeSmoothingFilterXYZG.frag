#version 330 core

uniform sampler2D qt_depth;  // THIS TEXTURE HOLDS THE DEPTH TEXTURE COORDINATES
uniform sampler2D qt_color;    // THIS TEXTURE HOLDS THE DEPTH TEXTURE COORDINATES
uniform       int qt_radius;   // THIS INTEGER HOLDS THE WINDOW RADIUS

layout(location = 0, index = 0) out vec4 qt_fragColor;

void main()
{
    float meanVec = 0.0;
    float valdVec = 0.0;
    int pixelCount = 0;
    for (int r = -qt_radius; r <= qt_radius; r++){
        for (int c = -qt_radius; c <= qt_radius; c++){
            // READ THE PIXEL FROM TEXTURE MEMORY
            float depth = texelFetch(qt_depth, ivec2(gl_FragCoord.x + c, gl_FragCoord.y + r), 0).r;
            float color = texelFetch(qt_color, ivec2(gl_FragCoord.x + c, gl_FragCoord.y + r), 0).r;

            // ADD THE PIXEL TO OUR ACCUMULATED SUM WHILE
            // KEEP RUNNING SUM OF NUMBER OF VALID PIXELS
            if (depth < 0.75 && color > 0.0){
                pixelCount++;
                meanVec += color * depth;
                valdVec += color;
            }
        }
    }

    if (pixelCount > 3){
        // DIVIDE BY THE NUMBER OF VALID PIXELS TO GET THE MEAN
        meanVec = meanVec / valdVec;

        // NOW USE THE STANDARD DEVIATION TO CHANGE THE LUMINANCE OF THE INCOMING COLOR
        qt_fragColor.a = float(texelFetch(qt_depth, ivec2(gl_FragCoord.x, gl_FragCoord.y), 0).r < 0.75);
        qt_fragColor.rgb = vec3(meanVec, meanVec, meanVec) / qt_fragColor.a;
    } else {
        qt_fragColor = vec4(0.0, 0.0, 0.0, 0.0);
    }
}
