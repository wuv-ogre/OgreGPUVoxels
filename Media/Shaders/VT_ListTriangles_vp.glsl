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

uniform sampler3D sDensity;

in vec3 vertex;
//in vec3 uv0;


ivec3 GetTexel(vec3 voxelCoord)
{
    return ivec3( voxelCoord * g_step_pixels + g_margin_pixels + g_quadrant_offset_pixels );
}

uint CubeCase(ivec3 texel)
{
	vec4 field0123;
	vec4 field4567;
    
    ivec2 sampler_density_tex_step = ivec2(g_step_pixels, 0);
    
    field0123.x = texelFetch(sDensity, texel + sampler_density_tex_step.yyy, 0).x;
	field0123.y = texelFetch(sDensity, texel + sampler_density_tex_step.xyy, 0).x;
	field0123.z = texelFetch(sDensity, texel + sampler_density_tex_step.yyx, 0).x;
	field0123.w = texelFetch(sDensity, texel + sampler_density_tex_step.xyx, 0).x;
	field4567.x = texelFetch(sDensity, texel + sampler_density_tex_step.yxy, 0).x;
	field4567.y = texelFetch(sDensity, texel + sampler_density_tex_step.xxy, 0).x;
	field4567.z = texelFetch(sDensity, texel + sampler_density_tex_step.yxx, 0).x;
	field4567.w = texelFetch(sDensity, texel + sampler_density_tex_step.xxx, 0).x;
    
    field0123 = clamp(field0123*99999.0, 0.0, 1.0);
    field4567 = clamp(field4567*99999.0, 0.0, 1.0);
    
    uvec4 i0123 = uvec4(uint(field0123.x), uint(field0123.y), uint(field0123.z), uint(field0123.w));
	uvec4 i4567 = uvec4(uint(field4567.x), uint(field4567.y), uint(field4567.z), uint(field4567.w));
	
	uint cube_case =	(i0123.x     ) | (i0123.y << 1u) | (i0123.z << 2u) | (i0123.w << 3u) |
    (i4567.x << 4u) | (i4567.y << 5u) | (i4567.z << 6u) | (i4567.w << 7u);
    
	return cube_case;
}

void SetVoxelCoord(inout uint result, vec3 coord)
{
	result = result | (uint(coord.z) << 12u) | (uint(coord.y) << 8u) | (uint(coord.x) << 4u);
}

void SetCubeCase(inout uint result, uint cube_case)
{
	result = result | (cube_case << 16u);
}

out block {
    uint Data;
} Voxel;

void main()
{
    ivec3 texel = GetTexel(vertex);
    uint cube_case = CubeCase(texel);
    
    Voxel.Data = 0u;
    SetVoxelCoord(Voxel.Data, vertex);
    SetCubeCase(Voxel.Data, cube_case);
}
