#version 150

in float oDepth;
out vec4 fragColour;

void main(void)
{
    float d = oDepth/5.0;
    fragColour = vec4(d,d,d, 1.0);
}