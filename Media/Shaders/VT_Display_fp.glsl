#version 150

in vec3 oNormal;
uniform vec4 highlight;
out vec4 fragColour;

void main(void)
{
    fragColour = vec4(oNormal*0.5 + 0.5 + highlight.xyz, 1.0);
}