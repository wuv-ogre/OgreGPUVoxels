#version 150

uniform vec3 g_world_space_chunk_size_lod_zero;
uniform sampler3D s_influence;
uniform vec4 highlight;

in VertexData
{
    vec3 viewPos;
    vec3 viewNormal;
    vec3 pos;
} v2f;


void DS_SetAlbedo(vec4 diffuse);
void DS_SetNormal(vec3 normal);
void DS_SetDepth(float depth);
void DS_SetMaterial(vec3 diffuse, vec3 specular, float shinness, vec3 ambient);

void main()
{
    // coords into a root level render volume
    vec3 uvw = mod( v2f.pos, g_world_space_chunk_size_lod_zero ) / ( g_world_space_chunk_size_lod_zero );
    uvw = vec3( texture(s_influence, uvw).x ) * 0.75 + 0.25;
    DS_SetAlbedo( vec4( uvw, 1.0 ) + highlight);
    DS_SetNormal(v2f.viewNormal);
    DS_SetDepth(length(v2f.viewPos));
    DS_SetMaterial(vec3(1.0,1.0,1.0), vec3(0.0,0.0,0.0), 0.431373, vec3(0.56, 0.56, 0.56));
}

