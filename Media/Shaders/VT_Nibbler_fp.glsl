#version 150

in vec3 wsCoord;
uniform vec3 g_nibbler_position;
uniform vec4 g_nibbler_attenuation;
uniform float g_nibbler_magnitude;

out float fragColour;

void main(void)
{
	fragColour = 1.0;
    
    vec3 toNibbler = wsCoord - g_nibbler_position;
    
	float lenSq = dot(toNibbler, toNibbler);
	float len = sqrt(lenSq);

    float attenuation = dot(g_nibbler_attenuation.yzw, vec3(1.0, len, lenSq));
    
    // without this we get separation at the seems
    if(len < g_nibbler_attenuation.x)
        fragColour = (1.0/attenuation) * g_nibbler_magnitude;
    else
        fragColour = 0.0;
    
    
    // dark point in the center of a red box
    //fragColour = 1.0 - ((1.0/attenuation) * g_nibbler_magnitude);
}
