#version 150

// constants
uniform float g_voxel_dim;                      // num cell corners
uniform float g_margin;
uniform float g_voxel_dim_minus_one;            // num cells
uniform float g_voxel_dim_minus_two;            // last cell index
uniform vec2 g_inv_voxel_dim;
uniform vec2 g_inv_voxel_dim_minus_one;
uniform vec2 g_voxel_dim_plus_margin;
uniform vec2 g_inv_voxel_dim_plus_margin;
uniform vec2 g_inv_voxel_dim_plus_margin_minus_one;
uniform vec2 g_inv_density_volume_size;
uniform float g_lod_count;
uniform float g_transition_width;				// [0.0-1.0]

// regular params
uniform vec3 g_world_space_chunk_pos;
uniform vec3 g_world_space_chunk_size;
uniform vec3 g_world_space_voxel_size;
uniform float g_lod_index;
uniform float g_lod_scale;                      // 1 / (1 << lod level)
uniform vec2 g_sampler_density_tex_step;        // lod_scale * inv_voxel_dim_plus_margin
uniform vec3 g_lod_quadrant_offset_uvw;

// transition params
uniform vec3 g_transition_face;                 // (1 0 0) or (0 1 0) or (0 0 1)
uniform vec3 g_one_minus_transition_face;
uniform float g_transition_face_side;			// 1 = positive face 0 = negtive
uniform float g_transition_face_idx;

uniform float g_step_pixels;                    // for 3 lods 4 2 1
uniform float g_margin_pixels;                  // for 3 lods is 4 for a margin value of 1
uniform vec3 g_quadrant_offset_pixels;

in uint vertex;
uniform sampler3D sDensity;
uniform sampler1D sTriTable;

out block {
    vec3 wsCoord[3];
    vec3 wsNormal[3];
#if INCLUDE_DELTA
    vec3 wsDelta[3];
#endif
} Voxel;

const ivec3 edge_start[12] = ivec3[12]
(
 ivec3(0, 0, 0), ivec3(0, 1, 0), ivec3(1, 0, 0), ivec3(0, 0, 0),
 ivec3(0, 0, 1), ivec3(0, 1, 1), ivec3(1, 0, 1), ivec3(0, 0, 1),
 ivec3(0, 0, 0), ivec3(0, 1, 0), ivec3(1, 1, 0), ivec3(1, 0, 0)
 );

/*
const ivec3 edge_dir[12] = ivec3[12]
(
 ivec3(0, 1, 0), ivec3(1, 0, 0), ivec3(0, 1, 0), ivec3(1, 0, 0),
 ivec3(0, 1, 0), ivec3(1, 0, 0), ivec3(0, 1, 0), ivec3(1, 0, 0),
 ivec3(0, 0, 1), ivec3(0, 0, 1), ivec3(0, 0, 1), ivec3(0, 0, 1)
 );
*/
const ivec3 edge_end[12] = ivec3[12]
(
 ivec3(0, 1, 0), ivec3(1, 1, 0), ivec3(1, 1, 0), ivec3(1, 0, 0),
 ivec3(0, 1, 1), ivec3(1, 1, 1), ivec3(1, 1, 1), ivec3(1, 0, 1),
 ivec3(0, 0, 1), ivec3(0, 1, 1), ivec3(1, 1, 1), ivec3(1, 0, 1)
 );


uvec3 GetVoxelCoord(uint src)
{
	// chunks limited to 16x16x16
	return ((uvec3(src) >> uvec3(4u,8u,12u)) & uint(0xF));
}

vec3 GetWSCoord(uvec3 voxelCoord)
{
    return g_world_space_chunk_pos + voxelCoord * g_inv_voxel_dim_minus_one.xxx * g_world_space_chunk_size;
}

ivec3 GetTexel(uvec3 voxelCoord)
{
    return ivec3( voxelCoord * g_step_pixels + g_margin_pixels + g_quadrant_offset_pixels );
}

vec3 GetUVW(uvec3 voxelCoord)
{
    // for pixel i, texcoord = (2i + 1)/(2N)
    return (2.0 * (voxelCoord * g_step_pixels + g_margin_pixels + g_quadrant_offset_pixels) + 1.0) * (0.5 * g_inv_density_volume_size.x);
}

uint GetCubeCase(uint src)
{
	return (src >> 16u);
}

uint GetPoly(uint src)
{
	return (src & uint(0xF));
}

uvec3 GetEdgeNumsForTriangle(uint triangle)
{
	// floating point 1D texture not range compressed
	return uvec3(texture(sTriTable, (float(triangle)+0.5)/1280.0).xyz + 0.25);
}

float Interpolate(ivec3 uvw_start, ivec3 uvw_end, uint iterations)
{
	float t = 0;
    vec3 str;
    str.yz = texelFetch(sDensity, uvw_end, 0).xx;
    str.x = texelFetch(sDensity, uvw_start, 0).x;
	uint i=0u;
    for(i=0u; i<iterations; i++)
    {
        ivec3 uvw_mid =(uvw_start + uvw_end)/2;
        str.y = texelFetch(sDensity, uvw_mid, 0).x;
        
        if (str.x/(str.x - str.y) < 1)
        {
			uvw_end = uvw_mid;
            str.z = str.y;
        }
        else
        {
			uvw_start = uvw_mid;
            str.x = str.y;
            t += pow(0.5, float(i + 1u));
        }
    }
    
    t += pow(0.5, float(i)) * clamp(str.x/(str.x - str.z), 0.0, 1.0);
    return t;
}

vec3 PlaceVertOnEdge(ivec3 texel, uint edgeNum)
{
    ivec3 edge_start = edge_start[edgeNum];
    ivec3 edge_end   = edge_end[edgeNum];
    
    ivec3 texel_start = texel + int(g_step_pixels) * edge_start;
    ivec3 texel_end   = texel + int(g_step_pixels) * edge_end;
    
	uint iterations = uint(g_lod_count) - uint(g_lod_index) - 1u;
    float t = Interpolate( texel_start, texel_end, iterations );
    
    return edge_start + t * (edge_end - edge_start);  // quicker without the array lookup?
}


vec3 ComputeNormal(vec3 uvw)
{
	vec3 grad;
	
	grad.x	=	texture(sDensity, uvw + g_inv_density_volume_size.xyy).x
              - texture(sDensity, uvw - g_inv_density_volume_size.xyy).x;
	grad.y	=	texture(sDensity, uvw + g_inv_density_volume_size.yxy).x
              - texture(sDensity, uvw - g_inv_density_volume_size.yxy).x;
	grad.z	=	texture(sDensity, uvw + g_inv_density_volume_size.yyx).x
              - texture(sDensity, uvw - g_inv_density_volume_size.yyx).x;
	
    grad /= g_world_space_voxel_size.xyz;
    
	return -normalize(grad);
}

vec3 ComputeDeltaForRegular(uvec3 voxelCoord, vec3 posWithinCell, vec3 normal)
{
    // 1 on edge coord 0 on inner cells
    vec3 faces = 1 - clamp(((g_voxel_dim_minus_two - voxelCoord) * voxelCoord) * 99999, 0.0, 1.0);
     
	// 1 on positive face
	vec3 side = (voxelCoord/g_voxel_dim_minus_two) * faces;
    
	vec3 delta = ((1-side) - posWithinCell) * g_transition_width * faces;
    
	delta *= g_world_space_voxel_size.xyz;
    
    return delta;
}

void main()
{
    uvec3 voxelCoord = GetVoxelCoord(vertex);
    
    vec3 wsCoord = GetWSCoord(voxelCoord);
    vec3 uvw = GetUVW(voxelCoord);
    ivec3 texel = GetTexel(voxelCoord);
    
    uint cube_case = GetCubeCase(vertex);

    // index to the triangle
	uint poly = GetPoly(vertex);
	uint triangle = cube_case*5u + poly;
    
    uvec3 edgeNums = GetEdgeNumsForTriangle(triangle);
    
    for(int i=0; i<3; i++)
    {
        uint edgeNum = edgeNums[i];
        
        vec3 posWithinCell = PlaceVertOnEdge(texel, edgeNum);
        
        Voxel.wsCoord[i] = wsCoord + posWithinCell * g_world_space_voxel_size.xyz;
        vec3 coord_uvw   = uvw     + posWithinCell * g_sampler_density_tex_step.xxx;
        
        Voxel.wsNormal[i] = ComputeNormal(coord_uvw);

#if INCLUDE_DELTA
        Voxel.wsDelta[i] = ComputeDeltaForRegular(voxelCoord, posWithinCell, Voxel.wsNormal[i]);
#endif
    }

    /*
    // check data structure
    for(int i=0; i<3; i++)
    {
        uint edgeNum = edgeNums[i];
        Voxel.wsCoord[i] = vec3(voxelCoord);
        
        Voxel.wsNormal[i] = vec3(cube_case, poly, edgeNum);
    }
    */
}
