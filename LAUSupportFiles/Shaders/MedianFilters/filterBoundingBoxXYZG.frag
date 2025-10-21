#version 330 core

uniform sampler2D qt_texture;   // THIS TEXTURE HOLDS THE RGB TEXTURE COORDINATES

uniform      mat4 qt_projection;     // THIS MATRIX IS THE PROJECTION MATRIX
uniform     float qt_xMin;
uniform     float qt_xMax;
uniform     float qt_yMin;
uniform     float qt_yMax;
uniform     float qt_zMin;
uniform     float qt_zMax;

layout(location = 0, index = 0) out vec4 qt_fragColor;

void main()
{
    // GET THE FRAGMENT PIXEL COORDINATE
    qt_fragColor = texelFetch(qt_texture, ivec2(gl_FragCoord.xy), 0);

    // PROJECT X,Y,Z COORDINATES TO THE GRAPHICS BUFFER
    vec4 pixel = qt_projection * vec4(qt_fragColor.xyz, 1.0);

    // SEE IF THE POSITION IS OUTSIDE THE BOUNDING BOX
    float flag = 1.0;
    if (pixel.x < qt_xMin){
        flag = 0.0;
    } else if (pixel.x > qt_xMax){
        flag = 0.0;
    } else if (pixel.y < qt_yMin){
        flag = 0.0;
    } else if (pixel.y > qt_yMax){
        flag = 0.0;
    } else if (pixel.z < qt_zMin){
        flag = 0.0;
    } else if (pixel.z > qt_zMax){
        flag = 0.0;
    }

    // SET UNRESOLVED PIXELS TO NAN
    qt_fragColor = ((qt_fragColor * flag) / flag) * (flag / flag);
}
