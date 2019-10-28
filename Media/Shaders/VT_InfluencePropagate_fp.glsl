#version 150

in vec3 oUv0;

uniform vec4 viewport_size;
uniform float g_decay;
uniform float g_momentum;
uniform bool g_preserve;

uniform sampler3D s_prev;

out float fragColour;

const ivec3 offsets[26] = ivec3[26]
(
 ivec3( -1, 0, 0 ),
 ivec3( 0, -1, 0 ),
 ivec3( 0, 0, -1 ),
 ivec3( 0, 0, 1 ),
 ivec3( 0, 1, 0 ),
 ivec3( 1, 0, 0 ),

 ivec3( -1, -1, 0 ),
 ivec3( -1, 0, -1 ),
 ivec3( -1, 0, 1 ),
 ivec3( -1, 1, 0 ),
 ivec3( 0, -1, -1 ),
 ivec3( 0, -1, 1 ),
 ivec3( 0, 1, -1 ),
 ivec3( 0, 1, 1 ),
 ivec3( 1, -1, 0 ),
 ivec3( 1, 0, -1 ),
 ivec3( 1, 0, 1 ),
 ivec3( 1, 1, 0 ),
 
 ivec3( -1, -1, -1 ),
 ivec3( -1, -1, 1 ),
 ivec3( -1, 1, -1 ),
 ivec3( -1, 1, 1 ),
 ivec3( 1, -1, -1 ),
 ivec3( 1, -1, 1 ),
 ivec3( 1, 1, -1 ),
 ivec3( 1, 1, 1 )
 );



void main(void)
{
    float stepSize = viewport_size.z;
    
    float inf = 0.0;
    
    for(int i=0; i<26; i++)
    {
        float val = texture( s_prev, oUv0 + offsets[i] * stepSize ).x * exp( -length( offsets[i] ) * g_decay );
        
        if( abs(val) > abs(inf) )
        {
            inf = val;
        }
    }
    
    /*
    // want to skip (0,0,0)
    for(int i=-1; i<=1; i++)
    {
        for(int j=-1; j<=1; j++)
        {
            for(int k=-1; k<=1; k++)
            {
                vec3 offset = vec3(i, j, k);
                
                float val = texture( s_prev, oUv0 + offset * stepSize ).x * exp( -length( offset ) * g_decay );
                
                if( abs(val) > abs(inf) )
                {
                    inf = val;
                }
            }
        }
    }
    */
    
    float prev = texture( s_prev, oUv0 ).x;

    if( g_preserve && ( abs(prev) > abs(inf) ) )
    {
        inf = prev;
    }
    else
    {
        inf = mix( prev, inf, g_momentum );
    }

    fragColour = inf;
    //fragColour = oUv0.y; //(oUv0.x + oUv0.y + oUv0.z) /3.0; // temp
}


