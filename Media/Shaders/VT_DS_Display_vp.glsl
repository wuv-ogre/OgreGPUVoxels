#version 150

in vec3 vertex;
in vec3 uv0;
in vec3 normal;

uniform mat4 worldviewproj_matrix;
uniform mat4 worldview_matrix;
uniform mat4 view_matrix;
uniform vec4 texel_offsets;

out VertexData
{
#if VTF_DEPTH
    vec2 depth;
#else
	vec3 viewPos;
    vec3 viewNormal;
    vec3 pos;
#endif
} v2f;



#if INCLUDE_DELTA

in vec3 uv1; // delta
uniform vec4 deltaFacesPos;	// custom param
uniform vec4 deltaFacesNeg;	// custom param

// x*1 + z*2 + 4*y = index
const ivec3 freedom_axis[8] = ivec3[8]
(
 ivec3(0, 0, 0), ivec3(1, 1, 0),
 ivec3(0, 1, 1), ivec3(1, 1, 1),
 ivec3(0, 1, 1), ivec3(1, 1, 1), ivec3(1, 1, 1), ivec3(1, 1, 1)
 );

vec3 ProjectToTrangentPlane(vec3 delta, vec3 N)
{
    vec3 N2 = N*N;
    mat3 m = mat3(1-N2.x,		-N.x*N.y,	-N.x*N.z,
                  -N.x*N.y,		1-N2.y,		-N.y*N.z,
                  -N.x*N.z,		-N.y*N.z,	1-N2.z);
    
    return m * delta;
}

#endif


void main()
{
    vec3 pos = vertex.xyz;
    vec3 normal = uv0;
    
#if INCLUDE_DELTA
    vec3 delta = uv1;
    
    ivec3 signDelta = sign(ivec3(delta));
    
    ivec3 face = clamp(max(ivec3(deltaFacesPos.xyz) * signDelta, 0) + max(ivec3(deltaFacesNeg.xyz) * -signDelta, 0), 0, 1);
    ivec3 freedom = freedom_axis[face.x + 2*face.z + 4*face.y];
    
    // as in Figure 4.13 a and b, forces corners into a point at the meeting points of chunks
    delta *= (1 - clamp((dot(signDelta, signDelta) -1)*99999.0, 0.0, 1.0));
    
    pos.xyz += ProjectToTrangentPlane(freedom * delta.xyz, normal.xyz);
#endif
    
    gl_Position = worldviewproj_matrix * vec4(pos, 1);

#if VTF_DEPTH
	gl_Position.xy += texel_offsets.zw * gl_Position.w;
    v2f.depth = gl_Position.zw;
#else
    v2f.viewPos = (worldview_matrix * vec4(pos, 1.0)).xyz;
    
    vec3 viewNormal = (view_matrix * vec4(normal, 0.0)).xyz;

    v2f.viewNormal = viewNormal;
    
    v2f.pos = pos;
#endif
}
