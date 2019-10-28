# OgreGPUVoxels

Voxel geometry generation on gpu by using 3D render volumes as the density function for Ogre3D v1.10 Xalafu.

The approach is (ignoring the technical details)

Divide the space into chucks or octantant chucks.

Fill render volume for low level chucks.

List triangles on gpu (i.e. cubecase).

iff a none zero vertex out count, gen triangles on gpu.

A higher level lod stratedy handles the creation and recreation of chucks, including transvoxel faces as needed for the transitions between different lods. Transitions are also created on gpu.

A single render volume per lowest lod level of a chuck allows for smoothing between transitions.

Note, InfluenceVolume is meant for 'directed' texturing and is experimental.

Demo

https://vimeo.com/368979548

The example is not usable in and of itself, rather its a guide as to how to use the lib and debug it

References

Voxel-Based Terrain for Real-Time Virtual Simulations -- LENGYEL

GPU GEMS 3 -- chapter 2
