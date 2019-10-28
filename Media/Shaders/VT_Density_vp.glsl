#version 150

uniform float g_voxel_dim;                    // num cell corners
uniform float g_margin;

uniform float g_voxel_dim_minus_one;          // num cells
uniform float g_voxel_dim_minus_two;          // last cell index
uniform vec3 g_world_space_chunk_pos;
uniform vec3 g_world_space_chunk_size;
uniform vec3 g_world_space_voxel_size;
uniform vec2 g_inv_voxel_dim;
uniform vec2 g_inv_voxel_dim_minus_one;
uniform vec2 g_voxel_dim_plus_margin;
uniform vec2 g_inv_voxel_dim_plus_margin;
uniform vec2 g_inv_voxel_dim_plus_margin_minus_one;
uniform vec3 g_world_space_chunk_size_lod_zero;

uniform float g_lod_scale;                      // 1 / (1 << lod level)
uniform float g_lod_count;
uniform float g_lod_index;
uniform vec2 g_inv_density_volume_size;
uniform vec2 g_sampler_density_tex_step;        // lod_scale * inv_voxel_dim_plus_margin
uniform vec3 g_transition_face;                 // (1 0 0) or (0 1 0) or (0 0 1)
uniform vec3 g_one_minus_transition_face;
uniform float g_transition_face_side;			// 1 = positive face 0 = negtive
uniform float g_transition_face_idx;
uniform float g_transition_width;				// [0.0-1.0]

uniform mat4 worldviewproj_matrix;
uniform float slice;

in vec4 vertex;
in vec2 uv0;

out vec3 wsCoord;

void main()
{
    gl_Position = worldviewproj_matrix * vertex;
    
    // chunkCoord is in [0..1] range
	vec3 chunkCoord = vec3(uv0, (slice /*+ 0.5*/) * g_inv_density_volume_size.x );

    chunkCoord.xyz *= g_voxel_dim * g_inv_voxel_dim_minus_one.x;

    // extChunkCoord goes outside that range, so we also compute some voxels outside of the chunk
    vec3 extChunkCoord = (chunkCoord * g_voxel_dim_plus_margin.x - g_margin) * g_inv_voxel_dim.x;
    
    wsCoord = g_world_space_chunk_pos + extChunkCoord * g_world_space_chunk_size_lod_zero;
}
