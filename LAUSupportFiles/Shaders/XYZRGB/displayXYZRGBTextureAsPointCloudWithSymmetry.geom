#version 330 core

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

uniform float qt_delta;      // USER DEFINED MAXIMUM DISTANCE THRESHOLD
in      float qt_z[];        // STORES THE Z VALUE OF THE INCOMING VERTICES
in       vec4 qt_geometry[]; // STORES THE TEXTURE INFORMATION
out      vec4 qt_fragment;   // OUTPUT TEXTURE VALUE

void main()
{
    // CALCULATE DISTANCE BETWEEN EACH VERTEX, AND IF THEY ARE ALL
    // INSIDE THE MAXIMUM DISTANCE THRESHOLD, THEN EMIT THE VERTICES
    // TO THE FRAGMENT SHADER FOR DISPLAY. OTHERWISE, CULL THEM.
    float threshold = qt_delta * abs(qt_z[0] + qt_z[1] + qt_z[2])/3.0;
    vec3 delta = vec3(distance(qt_z[0], qt_z[1]), distance(qt_z[1], qt_z[2]), distance(qt_z[2], qt_z[0]));
    if (all(lessThan(delta, vec3(threshold, threshold, threshold)))){
        qt_fragment = qt_geometry[0];
        gl_Position = gl_in[0].gl_Position;
        EmitVertex();

        qt_fragment = qt_geometry[1];
        gl_Position = gl_in[1].gl_Position;
        EmitVertex();

        qt_fragment = qt_geometry[2];
        gl_Position = gl_in[2].gl_Position;
        EmitVertex();
    }
    EndPrimitive();
}
