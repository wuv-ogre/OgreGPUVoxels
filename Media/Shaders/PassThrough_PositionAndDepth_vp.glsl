#version 150

in vec4 uv0;
in vec4 vertex;
out float oDepth;

uniform mat4 worldviewproj_matrix;

void main(void)
{
	gl_Position = worldviewproj_matrix * vertex;
	oDepth = abs(gl_Position.z);
}