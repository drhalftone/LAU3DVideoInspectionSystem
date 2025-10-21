#version 330 core

uniform sampler2D qt_depth;  // THIS TEXTURE HOLDS THE DEPTH TEXTURE COORDINATES
uniform sampler2D qt_color;    // THIS TEXTURE HOLDS THE DEPTH TEXTURE COORDINATES
uniform       int qt_radius;   // THIS INTEGER HOLDS THE WINDOW RADIUS

layout(location = 0, index = 0) out vec4 qt_fragColor;

void main()
{
    float meanVec = 0.0;
    float valdVec = 0.0;
    for (int r = -qt_radius; r <= qt_radius; r++){
        for (int c = -qt_radius; c <= qt_radius; c++){
            // READ THE PIXEL FROM TEXTURE MEMORY
            float pixel = texelFetch(qt_depth, ivec2(gl_FragCoord.x + c, gl_FragCoord.y + r), 0).r;

            // ADD THE PIXEL TO OUR ACCUMULATED SUM
            meanVec += pixel;

            // KEEP RUNNING SUM OF NUMBER OF VALID PIXELS
            valdVec += float(pixel < 1.0);
        }
    }

    // DIVIDE BY THE NUMBER OF VALID PIXELS TO GET THE MEAN
    meanVec = meanVec / valdVec;

    float stdVec = 0.0;
    for (int r = -qt_radius; r <= qt_radius; r++){
        for (int c = -qt_radius; c <= qt_radius; c++){
            // READ THE PIXEL FROM TEXTURE MEMORY
            float pixel = texelFetch(qt_depth, ivec2(gl_FragCoord.x + c, gl_FragCoord.y + r), 0).r;

            // ADD THE DISTANCE TO OUR ACCUMULATED STD PIXEL
            stdVec += ((pixel - meanVec) * (pixel - meanVec)) * float(pixel < 1.0);
        }
    }
    // DIVIDE BY THE NUMBER OF VALID PIXELS TO GET THE STANDARD DEVIATION
    stdVec = sqrt(stdVec / valdVec);

    // COPY MEAN VALUED PIXEL TO OUTPUT FRAGMENT
    float red = min(max(1.0 - (log(stdVec) + 8.3), 0.01), 1.0);

    // NOW USE THE STANDARD DEVIATION TO CHANGE THE LUMINANCE OF THE INCOMING COLOR
    qt_fragColor.rgb = vec3(red, red, red); //sqrt(texelFetch(qt_color, ivec2(gl_FragCoord.x, gl_FragCoord.y), 0).rgb + red);
    qt_fragColor.a = 1.0;
}
