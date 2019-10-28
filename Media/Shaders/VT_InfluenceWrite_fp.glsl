#version 150

uniform float g_influence;

out float fragColour;

void main(void)
{
    fragColour = g_influence;
}


