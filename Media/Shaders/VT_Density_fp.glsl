#version 150

in vec3 wsCoord;
out float fragColour;

// Declare external functions
float snoise(vec3 v);

float sigmoid(float z, float alpha){ return 1.0/(1.0+exp(alpha * z)); }

void main(void)
{
    float h = snoise(vec3(wsCoord.x* 0.01, 0.0, wsCoord.z * 0.01));
	fragColour = h*20 - wsCoord.y + 25.0;
    

    
    /*
    // cylinder wall
    {
        float noise0 = snoise(wsCoord * 0.01);

        float z = length(wsCoord.xz) - (1000.0 + noise0*20.0);
        fragColour = sigmoid(z, 0.05)*2.0 - 1.0;
    }
    
    // spiral
    {
        float a = 0.0015; // << radius
        float b = 0.008; // << slope
        float c = 0.12;  // << effects hight of tunnel
        float d = 150.0;  // << effects width of tunnel
        
        vec3 scale = vec3(a, b, a);
        
        vec3 p = wsCoord * scale;
        vec3 spiral;
        spiral.y = p.y;
        spiral.x = cos(p.y);
        spiral.z = sin(p.y);
        
        vec3 v = (spiral - p) / scale;
        
        float z = length(v.xz * c) - d;
        fragColour -= sigmoid(z, 0.1)*2.0 - 1.0;
    }
    */
}
