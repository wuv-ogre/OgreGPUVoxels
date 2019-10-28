#version 150

layout(points) in;
layout(points, max_vertices = 5) out;

in block {
    uint Data;
} Voxel[];


out uint oPos;

uint GetCubeCase(uint src)
{
	return src >> 16u;
}

void SetPoly(inout uint result, uint poly)
{
	result = result | poly;
}

const uint case_to_numpolys[256] = uint[256]
(
 0u, 1u, 1u, 2u, 1u, 2u, 2u, 3u,  1u, 2u, 2u, 3u, 2u, 3u, 3u, 2u,  1u, 2u, 2u, 3u, 2u, 3u, 3u, 4u,  2u, 3u, 3u, 4u, 3u, 4u, 4u, 3u,
 1u, 2u, 2u, 3u, 2u, 3u, 3u, 4u,  2u, 3u, 3u, 4u, 3u, 4u, 4u, 3u,  2u, 3u, 3u, 2u, 3u, 4u, 4u, 3u,  3u, 4u, 4u, 3u, 4u, 5u, 5u, 2u,
 1u, 2u, 2u, 3u, 2u, 3u, 3u, 4u,  2u, 3u, 3u, 4u, 3u, 4u, 4u, 3u,  2u, 3u, 3u, 4u, 3u, 2u, 4u, 3u,  3u, 4u, 4u, 5u, 4u, 3u, 5u, 2u,
 2u, 3u, 3u, 4u, 3u, 4u, 4u, 5u,  3u, 4u, 4u, 5u, 4u, 5u, 5u, 4u,  3u, 4u, 4u, 3u, 4u, 3u, 5u, 2u,  4u, 5u, 5u, 4u, 5u, 4u, 2u, 1u,
 1u, 2u, 2u, 3u, 2u, 3u, 3u, 4u,  2u, 3u, 3u, 4u, 3u, 4u, 4u, 3u,  2u, 3u, 3u, 4u, 3u, 4u, 4u, 5u,  3u, 4u, 4u, 5u, 4u, 5u, 5u, 4u,
 2u, 3u, 3u, 4u, 3u, 4u, 4u, 5u,  3u, 4u, 2u, 3u, 4u, 5u, 3u, 2u,  3u, 4u, 4u, 3u, 4u, 5u, 5u, 4u,  4u, 5u, 3u, 2u, 5u, 2u, 4u, 1u,
 2u, 3u, 3u, 4u, 3u, 4u, 4u, 5u,  3u, 4u, 4u, 5u, 2u, 3u, 3u, 2u,  3u, 4u, 4u, 5u, 4u, 3u, 5u, 4u,  4u, 5u, 5u, 2u, 3u, 2u, 4u, 1u,
 3u, 4u, 4u, 5u, 4u, 5u, 5u, 2u,  4u, 5u, 3u, 4u, 3u, 4u, 2u, 1u,  2u, 3u, 3u, 2u, 3u, 2u, 4u, 1u,  3u, 4u, 2u, 1u, 2u, 1u, 1u, 0u
);

void main()
{
    uint cube_case = GetCubeCase( Voxel[0].Data );
    
    if (cube_case * (255u-cube_case) > 0u)
	{
        uint num_polys = case_to_numpolys[cube_case];
        
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
