#version 150

in vec4 vertex;

uniform vec4 viewport_size;
uniform mat4 worldviewproj_matrix;
uniform float slice;

in vec2 uv0;
out vec3 oUv0;

void main()
{
    gl_Position = worldviewproj_matrix * vertex;
    oUv0 = vec3( uv0, slice * viewport_size.w );
}
