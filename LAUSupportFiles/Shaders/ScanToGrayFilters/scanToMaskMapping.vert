#version 330 core

uniform   vec2 qt_size;

uniform  float qt_fx;
uniform  float qt_cx;
uniform  float qt_fy;
uniform  float qt_cy;
uniform  float qt_k1;
uniform  float qt_k2;
uniform  float qt_k3;
uniform  float qt_p1;
uniform  float qt_p2;

out       vec2 qt_fragment;      // HOLDS THE UNDISTORTED PIXEL COORDINATE FOR THE FRAGMENT SHADER
in        vec2 qt_vertex;        // ATTRIBUTE WHICH HOLDS THE ROW AND COLUMN COORDINATES FOR THE CURRENT PIXEL

void main(void)
{
    // PASS THE VERTEX TO THE FRAGMENT SHADER
    qt_fragment = qt_vertex.xy / qt_size;

    // DERIVE THE DISTORTED PIXEL COORDINATE
    vec2 point = (qt_vertex.xy - vec2(qt_cx, qt_cy)) / qt_size;
    float rad = length(point) * length(point);

    // MAP THE DISTORTED TO THE UNDISTORTED PIXEL COORDINATE
    gl_Position.x = (qt_fragment.x - point.x * (qt_k1 * rad + qt_k2 * (rad * rad) + qt_k3 * (rad * rad * rad)) - (qt_p1 * (rad + 2 * (point.x * point.x)) + qt_p2 * point.x * point.y)) - 0.5;
    gl_Position.y = (qt_fragment.y - point.y * (qt_k1 * rad + qt_k2 * (rad * rad) + qt_k3 * (rad * rad * rad)) - (qt_p2 * (rad + 2 * (point.y * point.y)) + qt_p1 * point.x * point.y)) - 0.5;
    gl_Position.z = 0.0;
    gl_Position.a = 1.0;
}
