#version 330 core

in  vec4 qt_vertex;   // POINTS TO VERTICES PROVIDED BY USER ON CPU
out vec4 qt_coord;    // TEXTURE COORDINATE INTERPOLATED FOR FRAGMENT SHADER

void main(void)
{
    // COPY THE VERTEX COORDINATE TO THE GL POSITION
    gl_Position = qt_vertex;

    // GENERATE THE FRAGMENT COORDINATE FOR INTERPOLATION
    // IN THE RANGE FROM [0 1]
    qt_coord = 0.5*(gl_Position + 1.0);
}
