#version 330 core

uniform sampler2D qt_depthTexture;   // THIS TEXTURE HOLDS THE XYZ+TEXTURE COORDINATES
uniform sampler2D qt_colorTexture;   // THIS TEXTURE HOLDS THE XYZ+TEXTURE COORDINATES
uniform sampler2D qt_spherTexture;   // THIS TEXTURE HOLDS THE SPHERICAL TO CARTESION FACTORS

uniform      vec2 qt_depthLimits;    // THIS HOLDS THE RANGE LIMITS OF THE LOOK UP TABLE
uniform     float qt_scaleFactor;    // SCALE FACTOR FOR THE COLOR TEXTURE

layout(location = 0, index = 0) out vec4 qt_fragColor;

void main()
{
    // MAP THE FRAGMENT COORDINATES TO TEXTURE COORDINATES
    ivec2 textureCoordinate = ivec2(gl_FragCoord.x, gl_FragCoord.y);

    // CONVERT THE COLUMN COORDINATE TO PACKED COORDINATES
    int col = textureCoordinate.x/4;
    int chn = textureCoordinate.x%4;

    // USE THE FOLLOWING COMMAND TO GET Z FROM THE DEPTH BUFFER
    float phase = 100.0f * texelFetch(qt_depthTexture, ivec2(col, textureCoordinate.y), 0)[chn]; // * 6185.7 + 62.0; // * 7084.50 - 40.0;

    // USE THE FOLLOWING COMMAND TO GET FOUR FLOATS FROM THE TEXTURE BUFFER
    vec4 vecABCD = texelFetch(qt_spherTexture, ivec2(3*textureCoordinate.x + 0, textureCoordinate.y), 0);
    vec4 vecEFGH = texelFetch(qt_spherTexture, ivec2(3*textureCoordinate.x + 1, textureCoordinate.y), 0);
    vec4 vecIJKL = texelFetch(qt_spherTexture, ivec2(3*textureCoordinate.x + 2, textureCoordinate.y), 0);

    // DERIVE THE XYZ COORDINATES FROM THE PHASE VALUE
    qt_fragColor.b  = vecEFGH.x * phase * phase * phase * phase;
    qt_fragColor.b += vecEFGH.y * phase * phase * phase;
    qt_fragColor.b += vecEFGH.z * phase * phase;
    qt_fragColor.b += vecEFGH.w * phase;
    qt_fragColor.b += vecIJKL.x;

    qt_fragColor.g = (vecABCD.z * qt_fragColor.b + vecABCD.w);
    qt_fragColor.r = (vecABCD.x * qt_fragColor.b + vecABCD.y);

    // DERIVE CUMMULATIVE FLAG TERM
    float rngFlag = float(qt_fragColor.b > qt_depthLimits.x) * float(qt_fragColor.b < qt_depthLimits.y);

    // SET UNRESOLVED PIXELS TO NAN
    qt_fragColor = (qt_fragColor * rngFlag) / rngFlag * (rngFlag / rngFlag);

    // PASS THROUGH THE RGB TEXTURE
    qt_fragColor.a = pow(qt_scaleFactor * texelFetch(qt_colorTexture, textureCoordinate, 0).r, 0.5);

    // ROTATE THE IMAGE 180 DEGREES
    // qt_fragColor.y = -qt_fragColor.y;
}
