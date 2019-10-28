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
} Voxel;


const uint TransitionEdgeStartInd[20] = uint[20]
(
 0u, 0u, 0u, 1u, 1u, 2u, 2u, 3u, 3u, 4u, 4u, 5u, 6u, 6u, 7u, 8u, 9u, 9u, 10u, 11u
 );

const uint TransitionEdgeEndInd[20] = uint[20]
(
 1u, 3u, 9u, 2u, 4u, 5u, 10u, 4u, 6u, 5u, 7u, 8u, 7u, 11u, 8u, 12u, 10u, 11u, 12u, 12u
 );

// where value of one equals a half length edgeu, and a value of zero equals a full length
const uint TransitionHalfEdgeLength[20] = uint[20]
(
 1u, 1u, 0u, 1u, 1u, 1u, 0u, 1u, 1u, 1u, 1u, 1u, 1u, 0u, 1u, 0u, 0u, 0u, 0u, 0u
 );


/*
const int TransitionEdgeStartInd[20] = int[20]
(
 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 6, 6, 7, 8, 9, 9, 10, 11
 );

const int TransitionEdgeEndInd[20] = int[20]
(
 1, 3, 9, 2, 4, 5, 10, 4, 6, 5, 7, 8, 7, 11, 8, 12, 10, 11, 12, 12
 );

const int TransitionHalfEdgeLength[20] = int[20]
(
 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 0, 1, 0, 0, 0, 0, 0
 );
*/

const vec3 corners_transition[78] = vec3[78]
(
    vec3(0.0, 0.0, 1.0), vec3(0.0, 0.0, 0.5), vec3(0.0, 0.0, 0.0), vec3(0.0, 0.5, 1.0), vec3(0.0, 0.5, 0.5), vec3(0.0, 0.5, 0.0), vec3(0.0, 1.0, 1.0), vec3(0.0, 1.0, 0.5), vec3(0.0, 1.0, 0.0),
    vec3(1.0, 0.0, 1.0), vec3(1.0, 0.0, 0.0), vec3(1.0, 1.0, 1.0), vec3(1.0, 1.0, 0.0),
    
    vec3(1.0, 0.0, 0.0), vec3(1.0, 0.0, 0.5), vec3(1.0, 0.0, 1.0), vec3(1.0, 0.5, 0.0),	vec3(1.0, 0.5, 0.5), vec3(1.0, 0.5, 1.0), vec3(1.0, 1.0, 0.0), vec3(1.0, 1.0, 0.5),	vec3(1.0, 1.0, 1.0),
    vec3(0.0, 0.0, 0.0), vec3(0.0, 0.0, 1.0), vec3(0.0, 1.0, 0.0), vec3(0.0, 1.0, 1.0),
    
    vec3(0.0, 0.0, 1.0), vec3(0.5, 0.0, 1.0), vec3(1.0, 0.0, 1.0), vec3(0.0, 0.0, 0.5), vec3(0.5, 0.0, 0.5), vec3(1.0, 0.0, 0.5), vec3(0.0, 0.0, 0.0), vec3(0.5, 0.0, 0.0), vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 1.0), vec3(1.0, 1.0, 1.0), vec3(0.0, 1.0, 0.0), vec3(1.0, 1.0, 0.0),
    
    vec3(0.0, 1.0, 0.0), vec3(0.5, 1.0, 0.0), vec3(1.0, 1.0, 0.0), vec3(0.0, 1.0, 0.5), vec3(0.5, 1.0, 0.5), vec3(1.0, 1.0, 0.5), vec3(0.0, 1.0, 1.0), vec3(0.5, 1.0, 1.0), vec3(1.0, 1.0, 1.0),
    vec3(0.0, 0.0, 0.0), vec3(1.0, 0.0, 0.0), vec3(0.0, 0.0, 1.0), vec3(1.0, 0.0, 1.0),
    
    vec3(0.0, 0.0, 0.0), vec3(0.5, 0.0, 0.0), vec3(1.0, 0.0, 0.0), vec3(0.0, 0.5, 0.0), vec3(0.5, 0.5, 0.0), vec3(1.0, 0.5, 0.0), vec3(0.0, 1.0, 0.0), vec3(0.5, 1.0, 0.0), vec3(1.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0), vec3(1.0, 0.0, 1.0), vec3(0.0, 1.0, 1.0), vec3(1.0, 1.0, 1.0),
    
    vec3(0.0, 0.0, 1.0), vec3(0.5, 0.0, 1.0), vec3(1.0, 0.0, 1.0), vec3(0.0, 0.5, 1.0), vec3(0.5, 0.5, 1.0), vec3(1.0, 0.5, 1.0), vec3(0.0, 1.0, 1.0), vec3(0.5, 1.0, 1.0), vec3(1.0, 1.0, 1.0),
    vec3(0.0, 0.0, 0.0), vec3(1.0, 0.0, 0.0), vec3(0.0, 1.0, 0.0), vec3(1.0, 1.0, 0.0)
 );

// same as corners_transition but 9 10 11 12 indices have the same value as front face
const vec3 corners_transition_uvw[78] = vec3[78]
(
    vec3(0.0, 0.0, 1.0), vec3(0.0, 0.0, 0.5), vec3(0.0, 0.0, 0.0), vec3(0.0, 0.5, 1.0), vec3(0.0, 0.5, 0.5), vec3(0.0, 0.5, 0.0), vec3(0.0, 1.0, 1.0), vec3(0.0, 1.0, 0.5), vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0), vec3(0.0, 0.0, 0.0), vec3(0.0, 1.0, 1.0), vec3(0.0, 1.0, 0.0),
    
    vec3(1.0, 0.0, 0.0), vec3(1.0, 0.0, 0.5), vec3(1.0, 0.0, 1.0), vec3(1.0, 0.5, 0.0), vec3(1.0, 0.5, 0.5), vec3(1.0, 0.5, 1.0), vec3(1.0, 1.0, 0.0), vec3(1.0, 1.0, 0.5),	vec3(1.0, 1.0, 1.0),
    vec3(1.0, 0.0, 0.0), vec3(1.0, 0.0, 1.0), vec3(1.0, 1.0, 0.0), vec3(1.0, 1.0, 1.0),
    
    vec3(0.0, 0.0, 1.0), vec3(0.5, 0.0, 1.0), vec3(1.0, 0.0, 1.0), vec3(0.0, 0.0, 0.5), vec3(0.5, 0.0, 0.5), vec3(1.0, 0.0, 0.5), vec3(0.0, 0.0, 0.0), vec3(0.5, 0.0, 0.0), vec3(1.0, 0.0, 0.0),
    vec3(0.0, 0.0, 1.0), vec3(1.0, 0.0, 1.0), vec3(0.0, 0.0, 0.0), vec3(1.0, 0.0, 0.0),
    
    vec3(0.0, 1.0, 0.0), vec3(0.5, 1.0, 0.0), vec3(1.0, 1.0, 0.0), vec3(0.0, 1.0, 0.5), vec3(0.5, 1.0, 0.5), vec3(1.0, 1.0, 0.5), vec3(0.0, 1.0, 1.0), vec3(0.5, 1.0, 1.0), vec3(1.0, 1.0, 1.0),
    vec3(0.0, 1.0, 0.0), vec3(1.0, 1.0, 0.0), vec3(0.0, 1.0, 1.0), vec3(1.0, 1.0, 1.0),
    
    vec3(0.0, 0.0, 0.0), vec3(0.5, 0.0, 0.0), vec3(1.0, 0.0, 0.0), vec3(0.0, 0.5, 0.0), vec3(0.5, 0.5, 0.0), vec3(1.0, 0.5, 0.0), vec3(0.0, 1.0, 0.0), vec3(0.5, 1.0, 0.0), vec3(1.0, 1.0, 0.0),
    vec3(0.0, 0.0, 0.0), vec3(1.0, 0.0, 0.0), vec3(0.0, 1.0, 0.0), vec3(1.0, 1.0, 0.0),
    
    vec3(0.0, 0.0, 1.0), vec3(0.5, 0.0, 1.0), vec3(1.0, 0.0, 1.0), vec3(0.0, 0.5, 1.0), vec3(0.5, 0.5, 1.0), vec3(1.0, 0.5, 1.0), vec3(0.0, 1.0, 1.0), vec3(0.5, 1.0, 1.0), vec3(1.0, 1.0, 1.0),
    vec3(0.0, 0.0, 1.0), vec3(1.0, 0.0, 1.0), vec3(0.0, 1.0, 1.0), vec3(1.0, 1.0, 1.0)
);

const ivec3 i_corners_transition[78] = ivec3[78]
(
 ivec3(0, 0, 2), ivec3(0, 0, 1), ivec3(0, 0, 0), ivec3(0, 1, 2), ivec3(0, 1, 1), ivec3(0, 1, 0), ivec3(0, 2, 2), ivec3(0, 2, 1), ivec3(0, 2, 0),
 ivec3(2, 0, 2), ivec3(2, 0, 0), ivec3(2, 2, 2), ivec3(2, 2, 0),
 
 ivec3(2, 0, 0), ivec3(2, 0, 1), ivec3(2, 0, 2), ivec3(2, 1, 0),	ivec3(2, 1, 1), ivec3(2, 1, 2), ivec3(2, 2, 0), ivec3(2, 2, 1),	ivec3(2, 2, 2),
 ivec3(0, 0, 0), ivec3(0, 0, 2), ivec3(0, 2, 0), ivec3(0, 2, 2),
 
 ivec3(0, 0, 2), ivec3(1, 0, 2), ivec3(2, 0, 2), ivec3(0, 0, 1), ivec3(1, 0, 1), ivec3(2, 0, 1), ivec3(0, 0, 0), ivec3(1, 0, 0), ivec3(2, 0, 0),
 ivec3(0, 2, 2), ivec3(2, 2, 2), ivec3(0, 2, 0), ivec3(2, 2, 0),
 
 ivec3(0, 2, 0), ivec3(1, 2, 0), ivec3(2, 2, 0), ivec3(0, 2, 1), ivec3(1, 2, 1), ivec3(2, 2, 1), ivec3(0, 2, 2), ivec3(1, 2, 2), ivec3(2, 2, 2),
 ivec3(0, 0, 0), ivec3(2, 0, 0), ivec3(0, 0, 2), ivec3(2, 0, 2),
 
 ivec3(0, 0, 0), ivec3(1, 0, 0), ivec3(2, 0, 0), ivec3(0, 1, 0), ivec3(1, 1, 0), ivec3(2, 1, 0), ivec3(0, 2, 0), ivec3(1, 2, 0), ivec3(2, 2, 0),
 ivec3(0, 0, 2), ivec3(2, 0, 2), ivec3(0, 2, 2), ivec3(2, 2, 2),
 
 ivec3(0, 0, 2), ivec3(1, 0, 2), ivec3(2, 0, 2), ivec3(0, 1, 2), ivec3(1, 1, 2), ivec3(2, 1, 2), ivec3(0, 2, 2), ivec3(1, 2, 2), ivec3(2, 2, 2),
 ivec3(0, 0, 0), ivec3(2, 0, 0), ivec3(0, 2, 0), ivec3(2, 2, 0)
 );

// same as corners_transition but 9 10 11 12 indices have the same value as front face
const ivec3 i_corners_transition_uvw[78] = ivec3[78]
(
 ivec3(0, 0, 2), ivec3(0, 0, 1), ivec3(0, 0, 0), ivec3(0, 1, 2), ivec3(0, 1, 1), ivec3(0, 1, 0), ivec3(0, 2, 2), ivec3(0, 2, 1), ivec3(0, 2, 0),
 ivec3(0, 0, 2), ivec3(0, 0, 0), ivec3(0, 2, 2), ivec3(0, 2, 0),
 
 ivec3(2, 0, 0), ivec3(2, 0, 1), ivec3(2, 0, 2), ivec3(2, 1, 0), ivec3(2, 1, 1), ivec3(2, 1, 2), ivec3(2, 2, 0), ivec3(2, 2, 1),	ivec3(2, 2, 2),
 ivec3(2, 0, 0), ivec3(2, 0, 2), ivec3(2, 2, 0), ivec3(2, 2, 2),
 
 ivec3(0, 0, 2), ivec3(1, 0, 2), ivec3(2, 0, 2), ivec3(0, 0, 1), ivec3(1, 0, 1), ivec3(2, 0, 1), ivec3(0, 0, 0), ivec3(1, 0, 0), ivec3(2, 0, 0),
 ivec3(0, 0, 2), ivec3(2, 0, 2), ivec3(0, 0, 0), ivec3(2, 0, 0),
 
 ivec3(0, 2, 0), ivec3(1, 2, 0), ivec3(2, 2, 0), ivec3(0, 2, 1), ivec3(1, 2, 1), ivec3(2, 2, 1), ivec3(0, 2, 2), ivec3(1, 2, 2), ivec3(2, 2, 2),
 ivec3(0, 2, 0), ivec3(2, 2, 0), ivec3(0, 2, 2), ivec3(2, 2, 2),
 
 ivec3(0, 0, 0), ivec3(1, 0, 0), ivec3(2, 0, 0), ivec3(0, 1, 0), ivec3(1, 1, 0), ivec3(2, 1, 0), ivec3(0, 2, 0), ivec3(1, 2, 0), ivec3(2, 2, 0),
 ivec3(0, 0, 0), ivec3(2, 0, 0), ivec3(0, 2, 0), ivec3(2, 2, 0),
 
 ivec3(0, 0, 2), ivec3(1, 0, 2), ivec3(2, 0, 2), ivec3(0, 1, 2), ivec3(1, 1, 2), ivec3(2, 1, 2), ivec3(0, 2, 2), ivec3(1, 2, 2), ivec3(2, 2, 2),
 ivec3(0, 0, 2), ivec3(2, 0, 2), ivec3(0, 2, 2), ivec3(2, 2, 2)
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

uvec3 GetTransitionEdgeNumsForTriangle(uint triangle)
{
	// floating point 1D texture not range compressed
	return uvec3(texture(sTriTable, (float(triangle)+0.5)/6144.0).xyz + 0.25);
}

// For lod use the higher lods denisty values to place the vertex accurately,
// this means a sort of sub grid -> sub grid approach
void Interpolate(vec3 uvw_start, vec3 uvw_end, uint iterations, out float t)
{
	t = 0;
    vec3 str;
    str.yz = texture(sDensity, uvw_end).xx;
    str.x = texture(sDensity, uvw_start).x;
	uint i=0u;
    for(i=0u; i<iterations; i++)
    {
        vec3 uvw_mid =(uvw_start + uvw_end)/2;
        str.y = texture(sDensity, uvw_mid).x;
        
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
}

void Interpolate(ivec3 uvw_start, ivec3 uvw_end, uint iterations, out float t)
{
	t = 0;
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
}



// uvw The cells lower left in densityVol texture space
// edgeNum value [0..11] for the edge of the cube
/*
vec3 TransitionPlaceVertOnEdge(vec3 uvw, uint edgeNum)
{
	// could use the same setup as marching cubes PlaceVertOnEdge,
	// 5 * tables (edge start, edge end, edge dir, edge start uvw and  edge end uvw.)
	// edges * faces * num tables * float3 (20*6*5*3) = 1800 which is doable

    uint startIdx = TransitionEdgeStartInd[edgeNum] + uint(g_transition_face_idx)*13u;
    uint endIdx   = TransitionEdgeEndInd[edgeNum]   + uint(g_transition_face_idx)*13u;
    
    vec3 edge_start = corners_transition[startIdx];
    vec3 edge_end   = corners_transition[endIdx];
    vec3 edge_dir = edge_end - edge_start;

    vec3 uvw_edge_start = corners_transition_uvw[startIdx];
	vec3 uvw_edge_end   = corners_transition_uvw[endIdx];

    vec3 uvw_start = uvw + g_sampler_density_tex_step.xxx * uvw_edge_start;
    vec3 uvw_end   = uvw + g_sampler_density_tex_step.xxx * uvw_edge_end;
    
	float t = 0;
	uint iterations = uint(g_lod_count) - TransitionHalfEdgeLength[edgeNum] - uint(g_lod_index) - 1u;
	Interpolate(uvw_start, uvw_end, iterations, t);
    
	return edge_start + t*edge_dir;  // [0..1]
}
*/


// texel The cells lower left in densityVol texture pixels
// edgeNum value [0..11] for the edge of the cube
vec3 TransitionPlaceVertOnEdge(ivec3 texel, uint edgeNum)
{
    uint startIdx = TransitionEdgeStartInd[edgeNum] + uint(g_transition_face_idx)*13u;
    uint endIdx   = TransitionEdgeEndInd[edgeNum]   + uint(g_transition_face_idx)*13u;
    
    //ivec3 edge_start = i_corners_transition[startIdx];
    //ivec3 edge_end   = i_corners_transition[endIdx];
    //ivec3 edge_dir = edge_end - edge_start;
    vec3 edge_start = corners_transition[startIdx];
    vec3 edge_end   = corners_transition[endIdx];
    vec3 edge_dir = edge_end - edge_start;

    ivec3 i_uvw_edge_start = i_corners_transition_uvw[startIdx];
	ivec3 i_uvw_edge_end   = i_corners_transition_uvw[endIdx];
    
    ivec3 texel_start = texel + int(g_step_pixels/2) * i_uvw_edge_start;
    ivec3 texel_end   = texel + int(g_step_pixels/2) * i_uvw_edge_end;
    
	float t = 0;
	uint iterations = uint(g_lod_count) - TransitionHalfEdgeLength[edgeNum] - uint(g_lod_index) - 1u;
	Interpolate(texel_start, texel_end, iterations, t);
    
	return edge_start + t*edge_dir;  // [0..1]
    //return vec3(edge_start)/2.0 + t*vec3(edge_dir)/2.0;  // [0..1]
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
	
    grad *= g_world_space_voxel_size.xyz;
    
	return -normalize(grad);
}

vec3 ProjectToTrangentPlane(vec3 delta, vec3 N)
{
	vec3 N2 = N*N;
	mat3 m = mat3(1-N2.x,		-N.x*N.y,	-N.x*N.z,
                  -N.x*N.y,		1-N2.y,		-N.y*N.z,
                  -N.x*N.z,		-N.y*N.z,	1-N2.z);
    
	return m * delta;
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
	uint triangle = cube_case*12u + poly;
    
    uvec3 edgeNums = GetTransitionEdgeNumsForTriangle(triangle);
    
    for(int i=0; i<3; i++)
    {
        uint edgeNum = edgeNums[i];
        
        //vec3 posWithinCell = TransitionPlaceVertOnEdge(uvw, edgeNum);
        vec3 posWithinCell = TransitionPlaceVertOnEdge(texel, edgeNum);
        
        // uvw has to reference the outer edge so its value is the same as regular where the two cells meet
        vec3 uvwPosWithinCell = posWithinCell * g_one_minus_transition_face; // 0 face direction
        uvwPosWithinCell += g_transition_face * g_transition_face_side;

        vec3 coord_uvw   = uvw + uvwPosWithinCell * g_sampler_density_tex_step.xxx;
        vec3 normal = ComputeNormal(coord_uvw);
        
        vec3 delta = g_transition_face * (g_transition_face_side - posWithinCell)  * g_world_space_voxel_size.xyz;
        vec3 temp = ProjectToTrangentPlane(-delta, normal) * g_transition_width;
        delta = temp + delta;
        
        Voxel.wsCoord[i] = wsCoord + posWithinCell * g_world_space_voxel_size.xyz + delta;
        Voxel.wsNormal[i] = normal;
    }
    
}
