#version 150

layout(points) in;
layout(triangle_strip, max_vertices = 3) out;

in block {
    vec3 wsCoord[3];
    vec3 wsNormal[3];
#if INCLUDE_DELTA
    vec3 wsDelta[3];
#endif
} Voxel[];

out vec3 oPos;
out vec3 oUv0;

#if INCLUDE_DELTA
out vec3 oUv1;
#endif


void main()
{
    for(int i=0; i<3; i++)
    {
        oPos = Voxel[0].wsCoord[i];
        oUv0 = Voxel[0].wsNormal[i];
        
#if INCLUDE_DELTA
        oUv1 = Voxel[0].wsDelta[i];
#endif
        EmitVertex();
    }

    EndPrimitive();
}
