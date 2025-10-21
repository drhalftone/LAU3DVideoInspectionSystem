#version 330 core

uniform sampler2D qt_texture;        // THIS TEXTURE HOLDS THE XYZ+TEXTURE COORDINATES
uniform      mat4 qt_projection;     // THIS MATRIX IS THE PROJECTION MATRIX
uniform      mat4 qt_symmetry;       // THIS MATRIX IS THE PROJECTION MATRIX
uniform       int qt_mode;           // THIS FLAG TELLS US TO DISPLY TEXTURE OR SURFACE GRADIENT
uniform     float qt_scale = 10.0f;

in           vec2 qt_vertex;         // ATTRIBUTE WHICH HOLDS THE ROW AND COLUMN COORDINATES FOR THE CURRENT PIXEL
out          vec4 qt_geometry;       // COLOR OUTPUT TO GEOMETRY SHADER
out         float qt_z;              // STORES THE Z VALUE OF THE INCOMING VERTICES

void main(void)
{
    // USE THE FOLLOWING COMMAND TO GET FOUR FLOATS FROM THE TEXTURE BUFFER
    vec4 pixel = texelFetch(qt_texture, ivec2(qt_vertex), 0).rgba;

    // PROJECT X,Y,Z COORDINATES TO THE GRAPHICS BUFFER
    vec4 pointA = vec4(pixel.xyz, 1.0);
    gl_Position = qt_projection * pointA;

    if (qt_mode == 0){
        // READ THE COLOR VALUE FROM THE TEXTURE
        qt_geometry = vec4(pixel.aaa, 1.0);
    } else if (qt_mode == 1){
        // CALCULATE THE SURFACE NORMAL
        vec3 vecAB = (texelFetch(qt_texture, ivec2(qt_vertex)+ivec2(1,0), 0) - pointA).xyz;
        vec3 vecAC = (texelFetch(qt_texture, ivec2(qt_vertex)+ivec2(0,1), 0) - pointA).xyz;
        qt_geometry.rgb = normalize(cross(vecAC, vecAB));
    } else if (qt_mode == 2){
        // PASS THE XYZW VALUE AS THE COLOR FOR MOUSE CLICKS
        qt_geometry = pointA;
    } else if (qt_mode == 3){
        // PASS THE MODULO OF DEPTH FOR SANDBOXES
        qt_geometry = mod(pointA, qt_scale);
    } else if (qt_mode == 12){
        // PASS THE ROW COLUMN VALUE AS THE COLOR FOR MOUSE CLICKS
        qt_geometry = vec4(qt_vertex, pointA.a, pointA.r/pointA.r);
    }
    // SAVE THE Z VALUE BEFORE APPLYING PROJECTION MATRIX
    qt_z = pointA.z;

    // GET THE SYMMETRY COORDINATES
    vec4 sym = qt_symmetry * pointA;
    if (min(abs(sym.x), abs(sym.y)) < 3.0){
        qt_geometry.r = 1.0;
    }
}
