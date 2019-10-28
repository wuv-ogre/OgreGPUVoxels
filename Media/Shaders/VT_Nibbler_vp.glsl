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
uniform vec3 g_world_space_chunk_size_lod_zero;

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

in vec4 vertex;

uniform mat4 worldviewproj_matrix;
uniform float slice; // [0.0..1.0] here

out vec3 wsCoord;

void main()
{
    gl_Position = worldviewproj_matrix * vertex;
    
    vec3 chunkCoord = vec3((gl_Position.xy+1.0)*0.5, slice);
    //chunkCoord.y = 1.0 - chunkCoord.y;
    
    // chunkCoord is in [0..1] range
    
    chunkCoord.xyz *= g_voxel_dim * g_inv_voxel_dim_minus_one.x;
    
    vec3 extChunkCoord = (chunkCoord * g_voxel_dim_plus_margin.x - g_margin) * g_inv_voxel_dim.x;
    
    wsCoord = extChunkCoord * g_world_space_chunk_size_lod_zero;
}
