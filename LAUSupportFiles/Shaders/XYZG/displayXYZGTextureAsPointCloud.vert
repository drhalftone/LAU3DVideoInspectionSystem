#version 330 core

uniform sampler2D qt_texture;       // THIS TEXTURE HOLDS THE XYZ+TEXTURE COORDINATES
uniform      mat4 qt_projection;    // THIS MATRIX IS THE PROJECTION MATRIX
uniform       int qt_arg = 0;       // THIS FLAG TELLS US WE ARE DISPLAYING ON THE AUGMENTED REALITY PROJECTOR
uniform       int qt_mode;          // THIS FLAG TELLS US TO DISPLY TEXTURE OR SURFACE GRADIENT
uniform     float qt_scale;

in           vec2 qt_vertex;        // ATTRIBUTE WHICH HOLDS THE ROW AND COLUMN COORDINATES FOR THE CURRENT PIXEL
out          vec4 qt_geometry;      // COLOR OUTPUT TO GEOMETRY SHADER
out         float qt_z;             // STORES THE Z VALUE OF THE INCOMING VERTICES

void main(void)
{
    // USE THE FOLLOWING COMMAND TO GET FOUR FLOATS FROM THE TEXTURE BUFFER
    vec4 pointA = texelFetch(qt_texture, ivec2(qt_vertex), 0).rgba;
    if (qt_mode == 0){
        // READ THE COLOR VALUE FROM THE TEXTURE
        qt_geometry = vec4(pointA.aaa, 1.0);
    } else if (qt_mode == 1){
        // CALCULATE THE SURFACE NORMAL
        vec3 vecAB = (texelFetch(qt_texture, ivec2(qt_vertex)+ivec2(1,0), 0) - pointA).xyz;
        vec3 vecAC = (texelFetch(qt_texture, ivec2(qt_vertex)+ivec2(0,1), 0) - pointA).xyz;
        qt_geometry = vec4(normalize(cross(vecAC, vecAB)), 1.0);
    } else if (qt_mode == 2){
        // PASS THE XYZW VALUE AS THE COLOR FOR MOUSE CLICKS
        qt_geometry = vec4(pointA.rgb, pointA.r/pointA.r);
    } else if (qt_mode == 3){
        // PASS THE MODULO OF DEPTH FOR SANDBOXES
        qt_geometry = mod(abs(pointA), 15.0)/15.0; //qt_scale)/qt_scale;
    } else if (qt_mode == 12){
        // PASS THE ROW COLUMN VALUE AS THE COLOR FOR MOUSE CLICKS
        qt_geometry = vec4(qt_vertex, pointA.a, pointA.r/pointA.r);
    }

    // SAVE THE Z VALUE BEFORE APPLYING PROJECTION MATRIX
    qt_z = pointA.z;

    // CLEAR OUT THE TEXTURE COORDINATE AND REPLACE WITH 1.0
    pointA.a = 1.0;

    if (qt_arg == 1){
        // SET THE SCREEN POSIITON BASED ON PIXEL LOCATION
        gl_Position = qt_projection * vec4(qt_vertex, qt_z, 1.0);
    } else if (qt_arg == 2){
        // PROJECT THE POINT TO THE FIELD OF VIEW OF THE SANDBOX PROJECTOR
        vec4 point = qt_projection * pointA;

        // XYZ ARE THE HOMOGENEOUS PROJECTOR FIELD OF VIEW COORDINATES
        // SO DIVIDE X AND Y BY Z TO GET THE SCREEN COORDINATES IN THE RANGE FROM -1 TO +1
        gl_Position.xy = point.xy / point.z;

        // THE Z COORDINATE OF THE SCAN IS THEN IN THE RANGE OF -1 TO +1 IN THE W POSITION
        gl_Position.z = point.w;

        // SET THE W COORDINATE TO 1
        gl_Position.w = 1.0;
    } else {
        // PROJECT X,Y,Z COORDINATES TO THE GRAPHICS BUFFER
        gl_Position = qt_projection * pointA;
    }
}
