#version 330 core

uniform sampler2D qt_depthTexture; // THIS TEXTURE HOLDS THE DFT COEFFICIENTS AS A 3-D TEXTURE
uniform sampler2D qt_colorTexture; // THIS TEXTURE HOLDS THE DFT COEFFICIENTS AS A 3-D TEXTURE

uniform sampler2D qt_minTexture;   // THIS TEXTURE HOLDS THE MINIMUM XYZP TEXTURE
uniform sampler2D qt_maxTexture;   // THIS TEXTURE HOLDS THE MAXIMUM XYZP TEXTURE
uniform sampler3D qt_lutTexture;   // THIS TEXTURE HOLDS THE LOOK UP TABLE XYZP TEXTURE

uniform      vec2 qt_depthLimits;  // THIS HOLDS THE RANGE LIMITS OF THE LOOK UP TABLE
uniform     float qt_layers;       // THIS HOLDS THE NUMBER OF LAYERS IN THE LOOK UP TABLE

layout(location = 0, index = 0) out vec4 qt_fragColor;

void main()
{
    // MAP THE FRAGMENT COORDINATES TO TEXTURE COORDINATES
    ivec2 textureCoordinate = ivec2(gl_FragCoord.x/2, gl_FragCoord.y);

    // CONVERT THE COLUMN COORDINATE TO PACKED COORDINATES
    int col = textureCoordinate.x/4;
    int chn = textureCoordinate.x%4;

    // USE THE FOLLOWING COMMAND TO GET Z FROM THE DEPTH BUFFER
    float depthA = texelFetch(qt_depthTexture, ivec2(col, textureCoordinate.y), 0)[chn];

    // RESCALE THE PHASE TO BE IN THE SUPPLIED RANGE
    float depthB = (depthA - qt_depthLimits.x)/(qt_depthLimits.y - qt_depthLimits.x);

    // EXTRACT THE XYZ COORDINATE FROM THE LOOKUP TABLE
    float lambda = qt_layers * depthB - floor(qt_layers * depthB);

    // TEST THE PIXEL COORDINATE OF THE CURRENT FRAGMENT
    if (int(gl_FragCoord.x)%2 == 0){
        vec4 pixelA = texelFetch(qt_lutTexture, ivec3(2*textureCoordinate.x + 0, textureCoordinate.y, int(floor(qt_layers * depthB))), 0);
        vec4 pixelB = texelFetch(qt_lutTexture, ivec3(2*textureCoordinate.x + 0, textureCoordinate.y, int( ceil(qt_layers * depthB))), 0);

        // LINEARLY INTERPOLATE BETWEEN THE TWO LAYERS EXTRACTED FROM THE LOOK UP TABLE
        qt_fragColor = pixelA + lambda * (pixelB - pixelA);
    } else {
        vec4 pixelA = texelFetch(qt_lutTexture, ivec3(2*textureCoordinate.x + 1, textureCoordinate.y, int(floor(qt_layers * depthB))), 0);
        vec4 pixelB = texelFetch(qt_lutTexture, ivec3(2*textureCoordinate.x + 1, textureCoordinate.y, int( ceil(qt_layers * depthB))), 0);

        // LINEARLY INTERPOLATE BETWEEN THE TWO LAYERS EXTRACTED FROM THE LOOK UP TABLE
        qt_fragColor = texture(qt_colorTexture, (pixelA + lambda * (pixelB - pixelA)).xy, 0);
    }

    lambda = float((depthB > 0.0) && (depthB < 1.0));
    qt_fragColor = ((qt_fragColor * lambda)/lambda) * (lambda/lambda);
}
