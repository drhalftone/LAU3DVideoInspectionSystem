#version 330 core

uniform sampler2D qt_textureA;   // HOLDS THE CURRENT FRAME TEXTURE
uniform sampler2D qt_textureB;   // HOLDS THE PREVIOUS FRAME TEXTURE

layout(location = 0, index = 0) out vec4 qt_fragColor;

void main()
{
    // KEEP TRACK OF THE CURRENT PIXELS FRAGMENT COORDINATE
    ivec2 coord = ivec2(gl_FragCoord.xy);

    // KEEP TRACK OF THE BEST ERROR AND ITS CORRESPONDING OFFSET
    float bestError = 1000000.0;
    float zeroError = 0.0;
    vec2 bestOffst = ivec2(0,0);

    // SEARCH FOR THE BEST OFFSET WITHIN A 16X16 PIXEL WINDOW SURROUNDING CURRENT PIXEL
    for (int dC=-8; dC<9; dC++){
        for (int dR=-8; dR<9; dR++){
            // KEEP TRACK OF THE TOTAL ERROR FOR THE CURRENT WINDOW POSITION
            float error = 0.0;
            // SUM THE ERRORS FOR ALL PIXELS WITH A 9X9 WINDOW AROUND CURRENT PIXEL
            for (int dS=-4; dS<5; dS++){
                for (int dT=-4; dT<5; dT++){
                    // GET THE CURRENT PIXELS FROM THEIR TEXTURES
                    vec4 pixelA = texelFetch(qt_textureA, coord+ivec2(dS+dC, dT+dR), 0);
                    vec4 pixelB = texelFetch(qt_textureB, coord+ivec2(dS, dT), 0);

                    // UPDATE THE ERROR BASED ON THE DIFFERENCES IN COLOR
                    error += distance(pixelA.rgb, pixelB.rgb);

                    // KEEP TRACK OF THE ERROR FOR THE NO-MOTION CASE
                    if (dC == 0 && dR == 0){
                        zeroError += distance(pixelA.rgb, pixelB.rgb);
                    }
                }
            }

            // SEE IF THE NEW OFFSET IS BETTER THAN THE PREVIOUS
            if (error < bestError){
                bestError = error;
                bestOffst = ivec2(dC, dR);
            }
        }
    }

    // SET OUTPUT TO BEST OFFSET AND ITS CORRESPONDING ERROR
    qt_fragColor = vec4(float(bestOffst.x), float(bestOffst.y), bestError, zeroError);
}
