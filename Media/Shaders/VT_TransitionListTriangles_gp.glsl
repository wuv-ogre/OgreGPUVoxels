#version 150

layout(points) in;
layout(points, max_vertices = 5) out;

in block {
    uint Data;
} Voxel[];

uniform vec3 g_transition_face;                 // (1 0 0) or (0 1 0) or (0 0 1)

out uint oPos;

uint GetCubeCase(uint src)
{
	return src >> 16u;
}

void SetPoly(inout uint result, uint poly)
{
	result = result | poly;
}

/*
void SetTransitionPoly(inout uint result, uint poly)
{
	uint write_at = uint(g_transition_face.x*4 + g_transition_face.y*8 + g_transition_face.z*12);
	result = result | (poly << write_at);
}
*/

const uint case_to_numpolys_transition[512] = uint[512]
(
 0u, 2u, 1u, 3u, 2u, 4u, 3u, 3u,    1u, 3u, 2u, 4u, 3u, 5u, 4u, 4u,    2u, 4u, 3u, 5u, 4u, 6u, 5u, 5u,    3u, 5u, 4u, 6u, 3u, 5u, 4u, 4u,
 1u, 3u, 2u, 4u, 3u, 5u, 4u, 4u,    2u, 4u, 3u, 5u, 4u, 6u, 5u, 5u,    3u, 5u, 4u, 6u, 5u, 7u, 6u, 6u,    4u, 6u, 5u, 7u, 4u, 6u, 5u, 5u,
 2u, 4u, 3u, 5u, 4u, 6u, 5u, 5u,    3u, 5u, 4u, 6u, 5u, 7u, 6u, 6u,    4u, 6u, 5u, 7u, 6u, 6u, 7u, 5u,    5u, 7u, 6u, 8u, 5u, 5u, 6u, 4u,
 3u, 5u, 4u, 6u, 5u, 7u, 6u, 6u,    4u, 6u, 5u, 7u, 6u, 8u, 7u, 7u,    3u, 5u, 4u, 6u, 5u, 5u, 6u, 4u,    4u, 6u, 5u, 7u, 4u, 4u, 5u, 3u,
 1u, 3u, 2u, 4u, 3u, 5u, 4u, 4u,    2u, 4u, 3u, 5u, 4u, 6u, 5u, 5u,    3u, 5u, 4u, 6u, 5u, 7u, 6u, 6u,    4u, 6u, 5u, 7u, 4u, 6u, 5u, 5u,
 2u, 4u, 3u, 5u, 4u, 6u, 5u, 5u,    3u, 5u, 4u, 6u, 5u, 7u, 6u, 6u,    4u, 6u, 5u, 7u, 6u, 8u, 7u, 7u,    5u, 7u, 6u, 8u, 5u, 7u, 6u, 6u,
 3u, 3u, 4u, 4u, 5u, 5u, 6u, 4u,    4u, 4u, 5u, 5u, 6u, 6u, 7u, 5u,    5u, 5u, 6u, 6u, 7u, 5u, 8u, 4u,    6u, 6u, 7u, 7u, 6u, 4u, 7u, 3u,
 4u, 4u, 5u, 5u, 6u, 6u, 7u, 5u,    5u, 5u, 6u, 6u, 7u, 7u, 8u, 6u,    4u, 4u, 5u, 5u, 6u, 4u, 7u, 3u,    5u, 5u, 6u, 6u, 5u, 3u, 6u, 2u,
 2u, 4u, 3u, 5u, 4u, 6u, 5u, 5u,    3u, 5u, 4u, 6u, 5u, 7u, 4u, 4u,    4u, 6u, 5u, 7u, 6u, 8u, 7u, 7u,    5u, 7u, 6u, 8u, 5u, 7u, 4u, 4u,
 3u, 5u, 4u, 6u, 5u, 7u, 6u, 6u,    4u, 6u, 5u, 7u, 6u, 8u, 5u, 5u,    5u, 7u, 6u, 8u, 7u, 9u, 4u, 4u,    4u, 6u, 5u, 7u, 4u, 6u, 3u, 3u,
 4u, 6u, 5u, 7u, 6u, 8u, 7u, 7u,    5u, 7u, 6u, 8u, 7u, 9u, 6u, 6u,    6u, 8u, 7u, 9u, 8u, 12u, 9u, 7u,   7u, 9u, 8u, 6u, 7u, 7u, 6u, 4u,
 5u, 7u, 6u, 4u, 7u, 9u, 8u, 4u,    6u, 8u, 7u, 5u, 8u, 6u, 7u, 3u,    5u, 7u, 6u, 4u, 7u, 7u, 4u, 2u,    4u, 6u, 5u, 3u, 4u, 4u, 3u, 1u,
 3u, 5u, 4u, 4u, 5u, 7u, 6u, 4u,    4u, 6u, 5u, 5u, 6u, 4u, 5u, 3u,    5u, 7u, 6u, 6u, 7u, 9u, 8u, 6u,    6u, 8u, 7u, 7u, 6u, 4u, 5u, 3u,
 4u, 6u, 5u, 5u, 6u, 8u, 7u, 5u,    5u, 7u, 6u, 6u, 7u, 5u, 6u, 4u,    6u, 8u, 7u, 7u, 8u, 6u, 5u, 3u,    5u, 7u, 6u, 6u, 5u, 3u, 4u, 2u,
 5u, 5u, 6u, 4u, 7u, 7u, 8u, 4u,    6u, 6u, 7u, 5u, 8u, 4u, 7u, 3u,    7u, 7u, 8u, 6u, 9u, 7u, 6u, 4u,    4u, 4u, 5u, 3u, 4u, 2u, 3u, 1u,
 4u, 4u, 5u, 3u, 6u, 6u, 7u, 3u,    5u, 5u, 6u, 4u, 7u, 3u, 6u, 2u,    4u, 4u, 5u, 3u, 6u, 4u, 3u, 1u,    3u, 3u, 4u, 2u, 3u, 1u, 2u, 0u
 );


void main()
{
    uint cube_case = GetCubeCase( Voxel[0].Data );
    
	if (cube_case * (511u-cube_case) > 0u)
	{
        uint num_polys = case_to_numpolys_transition[cube_case];
        
        for(uint poly=0u; poly<num_polys; poly++)
		{
            uint result = Voxel[0].Data;
            SetPoly(result, poly);

            oPos = result;
            
            EmitVertex();
        }
    }

    EndPrimitive();
}
