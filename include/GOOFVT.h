#ifndef __GOOFVT_H
#define __GOOFVT_H

#include "GOOFInt3.h"
#include "GOOFStopwatch.h"
#include <Ogre.h>

namespace GOOF 
{
    class ProceduralRenderable;
    class Stopwatch;
    
    namespace VT
    {
        /**
         * Manager classes
         * The ProceduralRenderable in GenTriangles is different to the ProceduralRenderable in GenTrianglesDelta
         * must not mix them up if recycling ProceduralRenderables
         * Transitions cells are from the low lod to the high lod so highest lod has no transition cells
         */
        
        /**
         * Marching cubes gives us either 0 or 255 for the empty cube_case with a subtraction effect in the render volume if a chunk
         * was empty cube_case 0, there is no possibilty of generation new triangles, likewise an addition effect to the render volume 
         * where chunk is empty and all cells are 255 will not generate new triangles either. Essentially the sign of a single pixel in 
         * the render volume section of the chunk given we know it contains no triangles will tell us which of the subtraction or addition
         * effect could result in traingles and which couldn't.
        */
        
        class SharedParams
        {
        private:
            Ogre::GpuSharedParametersPtr mSharedParams;
            Ogre::uint8 mVoxelDim;
            Ogre::uint8 mMargin;
            Ogre::uint8 mLodCount;
            Ogre::Vector3 mLodZeroChunkSize; // world space size of a chunk
            Ogre::Vector3 mWorldSpaceToTextureSpaceScale;
            
        public:
            /**
             * Shaders limit the voxel grid to 16x16x16 hence voxelDim value of 16 is the maximum
             * Hardware may be optimized for texture sizes that are powers of 2, e.g might be good idea to use something like
             * voxelDim 14 lodCount 4 for a rvs size of 64
             */
            SharedParams(Ogre::Vector3 lodZeroChunkSize = Ogre::Vector3(100.0, 100.0, 100.0), Ogre::uint8 lodCount=3, Ogre::uint8 voxelDim=16, Ogre::uint8 margin=1);

            /**
             * Params that are typically set once
             * @param voxelDim Number of cell corners The number of cells is limited to 16 (4bits packing in shaders)
             * @param margin Value needs to be atleast 1 so normal can be calculated in the density volume
             * @param lodCount The number of lod levels
             * @param lodZeroChunkSize The world space size of a chunk in the lowest resolution (lod zero)
            */
            void setConstantParams(Ogre::Vector3 lodZeroChunkSize, Ogre::uint8 lodCount, Ogre::uint8 voxelDim, Ogre::uint8 margin);
            
            /**
             * Params that are typically set every time we build either a "regular chunk" or "a strip of transition cells"
             * @param index The index of the chunk
             * @param lodLevel The lod level the chunk is in
            */
            void setRegularParams(const Int3& index, Ogre::uint8 lodLevel);
            
            /**
             * Params that are set every time we build "transition cells"
             * @param face The face the transition is on (x,y,z) values [0,1,2]
             * @param side e.g positive or negative values [0,1]
            */
            void setTransitionParams(Ogre::uint8 face, Ogre::uint8 side);
            
            /**
             * @return The number of cell corners
            */
            const Ogre::uint8 getVoxelDim() const { return mVoxelDim; }
            
            /**
             * @return The number cells of margin data
            */
            const Ogre::uint8 getMargin() const { return mMargin; }
            
            const Ogre::uint8 getLodCount() const { return mLodCount; }
            
            const Ogre::Vector3 getLodChunkSize(const Ogre::uint8 lodLevel) const { return mLodZeroChunkSize / (1<<lodLevel); }
            
            const Ogre::Vector3 getLodVoxelSize(const Ogre::uint8 lodLevel) const { return getLodChunkSize(lodLevel) / (mVoxelDim - 1); }
            
            /**
             * There is one density volume for each zero lod chunk which covers all its octree quadrants of higher lod levels, allows
             * for more accurate interpolated vertex positions in the geometry shaders
            */
            const int getRenderVolumeSize() const { return (2*mMargin + mVoxelDim) * (1<<(mLodCount-1)); }
            
            /**
             * Scales a world space position to a texture scale
            */
            const Ogre::Vector3& getWorldSpaceToTextureSpaceScale() const { return mWorldSpaceToTextureSpaceScale; }
            
            /**
             * @param index The index of a root chunk
             * @return The world space position converted to texture space
            */
            Ogre::Vector3 toTextureSpace(const Int3& index, const Ogre::Vector3& worldPos) const
            {
                Ogre::Vector3 min = index.toVector3() * mLodZeroChunkSize - mMargin * getLodVoxelSize(0);
                Ogre::Vector3 result = (worldPos - min) * getWorldSpaceToTextureSpaceScale();
                return result;
            }
            
            /**
             * @param aab The chunks axis aligned box
            */
            void computeChunkBoundingBox(const Int3& index, const Ogre::uint8 lodLevel, Ogre::AxisAlignedBox& aab)
            {
                Ogre::Vector3 chunkSize = getLodChunkSize(lodLevel);
                Ogre::Vector3 pos = index.toVector3() * chunkSize;
                aab.setExtents(pos, pos + chunkSize);
            }
            
            void logParams(Ogre::String& log);
        };
        
        
        /**
         * Seed object used for building regular chunks
        */
        class RegularSeed
        {
        public:
            RegularSeed(Ogre::SceneManager* sceneMgr, const SharedParams* params);
            
            /**
             * destroys the seed object
            */
            ~RegularSeed();
            
            /**
             * call to rebuild this seed object (if we changed SharedParams voxel dims)
            */
            void build(const SharedParams* params);
            
            Ogre::ManualObject* mSeedObject;
        };
        
        /**
         * Seed object used for building transition strips
        */
        class TransitionSeed
        {
        public:
            TransitionSeed(Ogre::SceneManager* sceneMgr, const SharedParams* params);
            
            /**
             * destroys the seed object
            */
            ~TransitionSeed();

            /**
             * call to rebuild this seed object (if we changed SharedParams voxel dims)
            */
            void build(const SharedParams* params);
            
            Ogre::ManualObject* mSeedObject;
        };
        
        /**
         * Geometry shader writes out a single float vertex with the data we need to build the triangle
        */
        class ListTriangles
        {
        public:
            /**
             * @param seed e.g. from RegularSeed
            */
            ListTriangles(Ogre::SceneManager* sceneMgr, Ogre::ManualObject* seed, const SharedParams* params, const Ogre::String& materialName = "VT/ListTriangles");
            
            /**
             * Changes the density texture in the material
            */
            void setDensity(Ogre::TexturePtr tex);
            
            /**
             * @note remember to setDensity() prior to calling this
            */
            void update(Ogre::SceneManager* sceneMgr);
            void update(Ogre::SceneManager* sceneMgr, Stopwatch* stopwatch)
            {
                stopwatch->start();
                update(sceneMgr);
                stopwatch->stop();
            }
            
            /**
             * Check stream out query
            */
            size_t vertexCount();
            bool isEmpty() { return vertexCount() == 0; }
            
            /**
             * Unpacks the triangle data and writes it to the log it for debug
             * @param max The maximum number of triangles from the front of the buffer to log, a value of 0 will log all triangles.
            */
            virtual void log(Ogre::uint16 max = 50, const Ogre::String& text = "");
            
            ProceduralRenderable* mList;
            
        private:
            /**
             * No way to change the vertex buffer size without recreating the r2vb (reference counted destruction automatic)
            */
            Ogre::RenderToVertexBufferSharedPtr createRenderToVertexBuffer( Ogre::uint r2vbMaxVertexCount, const Ogre::String& materialName );
            
            void logTriangle(Ogre::uint& val);
            
            // each is twice the size of the previous r2vb
            std::vector<Ogre::RenderToVertexBufferSharedPtr> mr2vbs;
        };
        
        /**
         * Generates texture sTriTable used in GenTriangles shaders
        */
        class RegularTriTableTexture
        {
        public:
            RegularTriTableTexture();

            Ogre::TexturePtr mTriTable;
        };
        
        /**
         * Takes the triangle list as the input to the geometry shader and builds triangles
         * @note If the target rtvb doesn't exist/isn't the right size for the list creates a new rtvb thats 3* the max num triangles,
         * something to consider if we want to reuse ProceduralRenderable's and minimise the creation and destruction of RenderToVertexBuffer objects
        */
        class GenTriangles
        {
        public:
            /**
             * @param triTable e.g. from RegularTriTableTexture
            */
            GenTriangles(Ogre::TexturePtr triTable, const Ogre::String& materialName = "VT/GenTriangles");

            /**
             * Changes the density texture in the material
            */
            void setDensity(Ogre::TexturePtr tex);
            
            /**
             * @param list The object containing the list of triangles to build
             * @param target Where the triangles are built
             * @note remember to setDensity() prior to calling this
            */
            void update(Ogre::SceneManager* sceneMgr, ProceduralRenderable* list, ProceduralRenderable* target);
            void update(Ogre::SceneManager* sceneMgr, ProceduralRenderable* list, ProceduralRenderable* target, Stopwatch* stopwatch)
            {
                stopwatch->start();
                update(sceneMgr, list, target);
                stopwatch->stop();
            }
            
            /**
             * Writes the triangle vertex data to the log for debug
             * @param max The maximum number of triangles from the front of the buffer to log, a value of 0 will log all triangles.
            */
            void log(ProceduralRenderable* target, Ogre::uint16 max = 50, const Ogre::String& text = "");
            
        protected:
            /**
             * No way to change the vertex buffer size without recreating the r2vb (reference counted destruction automatic)
             */
            Ogre::RenderToVertexBufferSharedPtr createRenderToVertexBuffer( Ogre::uint r2vbMaxVertexCount );
            
            /**
             * Added so GenTrianglesDelta can add an element for the delta
            */
            virtual void addToVertexDeclartion(Ogre::VertexDeclaration* vertexDecl, Ogre::uint& offset){}
            
        protected:
            Ogre::MaterialPtr mGenTrianglesMat;
        };
        
        /**
         * Takes the triangle list as the input to the geometry shader and builds triangles, BUT with the outer cells having an optional offset
         * this is to allow the transition strips to be slotted in when needed.
         * Highest lod will use GenTriangles all other lods will use GenTrianglesDelta
        */
        class GenTrianglesDelta : public GenTriangles
        {
        public:
            GenTrianglesDelta(Ogre::TexturePtr triTable, const Ogre::String& materialName = "VT/GenTrianglesDelta");
            
        protected:
            /**
             * Adds an extra VES_FLOAT3 for the delta
            */
            virtual void addToVertexDeclartion(Ogre::VertexDeclaration* vertexDecl, Ogre::uint& offset);
        };
        
        /**
         * Geometry shader writes out a single float vertex with the data we need to build the transition triangle
         */
        class TransitionListTriangles : public ListTriangles
        {
        public:
            /**
             * @param seed e.g. from TransitionSeed
            */
            TransitionListTriangles(Ogre::SceneManager* sceneMgr, Ogre::ManualObject* seed, const SharedParams* params, const Ogre::String& materialName = "VT/TransitionListTriangles");
        };
        
        /**
         * Generates texture sTriTable used in TransitionGenTriangles shaders
         */
        class TransitionTriTableTexture
        {
        public:
            TransitionTriTableTexture();
            
            Ogre::TexturePtr mTriTable;
        };
        
        /**
         * Takes the triangle list as the input to the geometry shader and builds triangles for a transition
        */
        class TransitionGenTriangles : public GenTriangles
        {
        public:
            /**
             * @param triTable e.g. from TransitionTriTableTexture
            */
            TransitionGenTriangles(Ogre::TexturePtr triTable, const Ogre::String& materialName = "VT/TransitionGenTriangles");
        };

        /**
         * A single ray at polygon level (tests all polygons, should use only for long ray query e.g. mouse ray)  
        */
        class RayCast
        {
        public:
            RayCast(Ogre::SceneManager* sceneMgr, const SharedParams* params, const Ogre::String& materialName = "VT/RayCast");
            
            void update(Ogre::SceneManager* sceneMgr, ProceduralRenderable* triangles, const Ogre::Ray& ray, Ogre::Real& nearest);
            void update(Ogre::SceneManager* sceneMgr, ProceduralRenderable* triangles, const Ogre::Ray& ray, Ogre::Real& nearest, Stopwatch* stopwatch)
            {
                stopwatch->start();
                update(sceneMgr, triangles, ray, nearest);
                stopwatch->stop();
            }

        private:
            ProceduralRenderable* mHitPoints;
        };
        
        
        /**
         * Agents over terrain (many short length rays)
         * Each agent would store its last triangle, or triangles in a certain range? 
         * when position changes check this list first.
        */
    }
    
}
#endif
