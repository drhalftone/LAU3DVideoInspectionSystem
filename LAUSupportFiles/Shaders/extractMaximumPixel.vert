#version 330 core

in  vec4 qt_vertex;   // POINTS TO VERTICES PROVIDED BY USER ON CPU

void main(void)
{
    // SIMPLY PASS THE INCOMING VERTEX TO THE SCREEN
    gl_Position = qt_vertex;
}
