#version 330 core

uniform float qt_radius;     // THIS FLOAT HOLDS THE RADIUS OF THE FIDUCIAL CUBES
uniform  mat4 qt_projection; // THIS MATRIX IS THE PROJECTION MATRIX
uniform  vec3 qt_fiducial;   // THIS VECTOR HOLDS THE COORDINATE OF THE CURRENT FIDUCIAL
uniform   int qt_arg = 0;    // THIS FLAG TELLS US WE ARE DISPLAYING ON THE AUGMENTED REALITY PROJECTOR

in  vec3 qt_vertexA;         // POINTS TO VERTICES PROVIDED BY USER ON CPU
in  vec2 qt_vertexB;         // POINTS TO VERTICES PROVIDED BY USER ON CPU

out vec2 qt_texCoord;        // TEXTURE COORDINATE FOR RESCALING THE TEXTURE TO FIT THE SCREEN

void main(void)
{
    if (qt_arg == 2){
        // PROJECT THE POINT TO THE FIELD OF VIEW OF THE SANDBOX PROJECTOR
        vec4 point = qt_projection * vec4(qt_radius * qt_vertexA + qt_fiducial, 1.0);

        // XYZ ARE THE HOMOGENEOUS PROJECTOR FIELD OF VIEW COORDINATES
        // SO DIVIDE X AND Y BY Z TO GET THE SCREEN COORDINATES IN THE RANGE FROM -1 TO +1
        gl_Position.xy = point.xy / point.z;

        // THE Z COORDINATE OF THE SCAN IS THEN IN THE RANGE OF -1 TO +1 IN THE W POSITION
        gl_Position.z = point.w;

        // SET THE W COORDINATE TO 1
        gl_Position.w = 1.0;
    } else {
        // SIMPLY PASS THE INCOMING VERTEX TO THE SCREEN
        gl_Position = qt_projection * vec4(qt_radius * qt_vertexA + qt_fiducial, 1.0);
    }

    // CALCULATE THE TEXTURE COORDINATE FROM THE VERTEX COORDINATE
    qt_texCoord = qt_vertexB;
}
