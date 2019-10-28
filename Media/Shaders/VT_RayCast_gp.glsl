#version 150

layout(triangles) in;
layout(points, max_vertices = 1) out;

uniform vec3 rayOrigin;
uniform vec3 rayDir;
uniform float rayLength;

out float oPos;

bool intersects (const vec3 rayOrigin, const vec3 rayDir,
				 const vec3 a, const vec3 b, const vec3 c,
				 out float distance)
{
    distance = 0.0;
    
    vec3 normal = cross(b-a, c-a);
    
    const float EPSILON = 1e-6f;
	
	// Calculate intersection with plane.
	float t;
    {
		float denom = dot(normal, rayDir);
        
		if(abs(denom) < EPSILON)
		{
			return false;
		}
		t = dot(normal, a - rayOrigin) / denom;
        
		if (t < 0)
		{
            // Intersection is behind origin
			return false;
		}
	}
	
    // Calculate the largest area projection plane in X, Y or Z.
    int i0, i1;
    {
		float n0 = abs(normal[0]);
		float n1 = abs(normal[1]);
		float n2 = abs(normal[2]);
        
		i0 = 1; i1 = 2;
        if (n1 > n2)
		{
			if (n1 > n0)
                i0 = 0;
        }
        else
        {
			if (n2 > n0)
                i1 = 0;
	    }
	}
	
    // Check the intersection point is inside the triangle.
    {
		float u1 = b[i0] - a[i0];
		float v1 = b[i1] - a[i1];
		float u2 = c[i0] - a[i0];
		float v2 = c[i1] - a[i1];
		float u0 = t * rayDir[i0] + rayOrigin[i0] - a[i0];
		float v0 = t * rayDir[i1] + rayOrigin[i1] - a[i1];
        
		float alpha = u0 * v2 - u2 * v0;
		float beta  = u1 * v0 - u0 * v1;
		float area  = u1 * v2 - u2 * v1;
        
		float tolerance = - EPSILON * area;
        
		if (area > 0)
		{
			if (alpha < tolerance || beta < tolerance || alpha+beta > area-tolerance)
				return false;
		}
        else
		{
			if (alpha > tolerance || beta > tolerance || alpha+beta < area-tolerance)
				return false;
		}
	}
    
	distance = t;
	return true;
}

const float EPSILON = 1e-30;

void main()
{
    float dist = 0.0;
    if(intersects(rayOrigin, rayDir,
                 gl_in[0].gl_Position.xyz,  gl_in[1].gl_Position.xyz,  gl_in[2].gl_Position.xyz,
                 dist) /*&& dist < rayLength*/)
    {
        oPos = dist;
        EmitVertex();
    }

    
    /*
    vec3 v1 = (gl_in[1].gl_Position.xyz - gl_in[0].gl_Position.xyz).xyz;
	vec3 v2 = (gl_in[2].gl_Position.xyz - gl_in[0].gl_Position.xyz).xyz;
	vec3 pvec = cross(rayDir, v2);
	float det = dot(v1, pvec);
	if ((det<EPSILON) && (det>-EPSILON))
        return; // vector is parallel to triangle's plane
    
	float invDet = 1.0 / det;
    vec3 tvec = rayOrigin - gl_in[0].gl_Position.xyz;
	float u = dot(tvec, pvec)*invDet;
	if ((u<0.0) || (u>1.0))
        return;
    
	vec3 qvec = cross(tvec, v1);
	float v = dot(rayDir, qvec)*invDet;
	if ((v<0.0) || (u+v>1.0))
        return;
    
	float t = dot(v2, qvec)*invDet;
	if (t<=0)
        return;
    
    oPos = t;
    EmitVertex();
    EndPrimitive();
    */
}
