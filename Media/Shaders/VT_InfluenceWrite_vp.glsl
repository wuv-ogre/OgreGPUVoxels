#version 150

in vec4 vertex;

uniform mat4 worldviewproj_matrix;

void main()
{
	gl_Position = worldviewproj_matrix * vertex;
}
