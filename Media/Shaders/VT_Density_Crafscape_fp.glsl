#version 150

in vec3 wsCoord;

uniform sampler2D heights;

out float fragColour;

float sigmoid(float z, float alpha){ return 1.0/(1.0+exp(alpha * z)); }

void main(void)
{
    float worldWidth = 800.0*4;
    vec2 uv = vec2(wsCoord.x * 1.0/worldWidth, wsCoord.z * 1.0/worldWidth) + vec2(0.5, 0.5);
    float h = texture(heights, uv).x;
    float z = h*2000 - wsCoord.y + 25.0;
    fragColour = (sigmoid(z, 1.0)*2.0 - 1.0);

    //fragColour = ((-180 - wsCoord.y)* 0.01);
}
