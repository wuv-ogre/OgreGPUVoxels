#ifndef __GOOFVTOctreeChunk_H
#define __GOOFVTOctreeChunk_H

#include "GOOFSharedData.h"
#include "GOOFVT.h"
#include "GOOFRenderTexture.h"
#include <Ogre.h>

namespace GOOF 
{
    class RenderVolume;
    class ProceduralRenderable;

    namespace VT
    {
        /**
         * what we really need is a timestamp for each section of the render volume then Octree chunk will know if its section of the render volume has changed
         */
        class RenderVolumeWithTimeStamp : public RenderVolume
        {
        public:
            RenderVolumeWithTimeStamp(Ogre::Camera* camera, const Ogre::String &name, const Ogre::String &group, uint width, uint height, uint depth, Ogre::PixelFormat format = Ogre::PF_FLOAT32_RGBA) : RenderVolume(camera, name, group, width, height, depth, format), mTimeStamp(0)
            {
            }

            virtual void render(Ogre::SceneManager* sceneMgr, Ogre::Pass* pass)
            {
                RenderVolume::render(sceneMgr, pass);
                mTimeStamp = Ogre::Root::getSingleton().getTimer()->getMicroseconds();
            }

            virtual void renderSection(Ogre::SceneManager* sceneMgr, Ogre::Pass* pass, const Ogre::Vector3& texPos, const Ogre::Vector3 texScale)
            {
                RenderVolume::renderSection(sceneMgr, pass, texPos, texScale);
                mTimeStamp = Ogre::Root::getSingleton().getTimer()->getMicroseconds();
            }
            
            Ogre::uint64 mTimeStamp;
        };
        
        class OctreeChunk
        {
        public:
            // Index within the lod level
            GOOF::Int3 mIndex;
            
            // Lod level, zero is the lowest
            Ogre::uint8 mLodLevel;
            
            // Lod one lower or NULL if lod level zero
            OctreeChunk* mParent;
            
            // Store root or use recursive methods?
            // OctreeChunk* mRoot;
            
            // Sibling chunks across each chunks face x, -x, y, -y, z, -z
            OctreeChunk* mSibling[6];
            
            // 8 child quadrants, Null where nothing generated or not generated yet
            OctreeChunk* mChildren[8];
            
            // This chunk
            ProceduralRenderable* mRenderable;
            
            // 6 transitions across the faces of the chunk, Null where nothing generated or not generated yet
            ProceduralRenderable* mTransition[6];
            
            // Records when the chunk was created/recreated
            Ogre::uint64 mTimeStamp;
            
            // Records when the transition chunks were created
            Ogre::uint64 mTransitionTimeStamp[6];
            
            // Debugging
            Ogre::WireBoundingBox* mWireBoundingBox;
            
            // stored as a single ListTriangles is reused
            size_t mListTrianglesVertexCount;

        public:
            OctreeChunk();
            virtual ~OctreeChunk();
            
            OctreeChunk* getRoot();
            virtual Ogre::SceneNode* getSceneNode() const { return mParent->getSceneNode(); }
            virtual RenderVolumeWithTimeStamp* getDensity() const { return mParent->getDensity(); }
            virtual Ogre::MaterialPtr getDisplayMat() const { return mParent->getDisplayMat(); }
            virtual Ogre::MaterialPtr getDisplayDeltaMat() const { return mParent->getDisplayDeltaMat(); }
            
            /**
             * sets the visible state of the procedural renderable
            */
            void setRenderableVisible( bool visible );
            
            /**
             * bug : if we change a texture link in the materials we have to reapply those materials or the texture won't get updated...
            */
            void reapplyMaterials();
            
            Ogre::WireBoundingBox* getWireBoundingBox();
            
            void hideAllChildren();
        };
        
        class RootOctreeChunk : public OctreeChunk
        {
        public:
            // scene node this and all child OctreeChunk renderables attach to
            Ogre::SceneNode* mSceneNode;

            // holds the density this and all child chunks use
            RenderVolumeWithTimeStamp* mDensity;
            
            // default same as OctreeChunkManager
            Ogre::MaterialPtr mDisplayMat;
            
            // default same as OctreeChunkManager
            Ogre::MaterialPtr mDisplayDeltaMat;

        public:
            RootOctreeChunk() : OctreeChunk() {}
            virtual ~RootOctreeChunk() {}

            virtual Ogre::SceneNode* getSceneNode() const { return mSceneNode; }
            virtual RenderVolumeWithTimeStamp* getDensity() const { return mDensity; }
            virtual Ogre::MaterialPtr getDisplayMat() const { return mDisplayMat; }
            virtual Ogre::MaterialPtr getDisplayDeltaMat() const { return mDisplayDeltaMat; }
        };
        
        
        class OctreeChunkManagerPolicy
        {
        protected:
            bool mCull;      // if true then there is no attempt for an empty chunk to create children
            SharedParams* mSharedParams;
            
        public:
            OctreeChunkManagerPolicy( SharedParams* sharedParams, bool cull ) : mSharedParams(sharedParams),  mCull(cull) {}
            
            const bool doCulling() { return mCull; }
            
            virtual bool doCreate( const Int3& index, Ogre::uint8 lodLevel )
            {
                return true;
            }
            
            virtual bool doChildren( const Int3& index, Ogre::uint8 lodLevel, bool empty )
            {
                return !(empty && mCull);
            }
        };
        
        /**
         * Only chunks within the spheres radius are created, possible use with 'Nibbler'
        */
        class OctreeChunkManagerSpherePolicy : public OctreeChunkManagerPolicy
        {
        protected:
            Ogre::Sphere mSphere;
            
        public:
            OctreeChunkManagerSpherePolicy( SharedParams* sharedParams, bool cull ) : OctreeChunkManagerPolicy(sharedParams, cull) {}

            Ogre::Sphere& sphere() { return mSphere; }
            
            virtual bool doCreate( const Int3& index, Ogre::uint8 lodLevel )
            {
                Ogre::AxisAlignedBox aab;
                mSharedParams->computeChunkBoundingBox(index, lodLevel, aab);
                return aab.distance( mSphere.getCenter() ) < mSphere.getRadius();
            }
        };

        
        /**
         * Create all root chunks and store them. If the chunk contains no cells then OctreeChunk simply contains the rvs.
         * Could inherit SharedParams since any change to them requires we destroy all chunks
        */
        class OctreeChunkManager
        {
        public:
            typedef std::map<Int3, OctreeChunk*, Int3Less> ChunkMap;
            typedef Ogre::MapIterator<ChunkMap> ChunkIterator;

        protected:
            Ogre::SceneManager* mSceneMgr;
            SharedParams* mSharedParams;
            Ogre::Camera* mCamera;
            Ogre::MaterialPtr mDensityMat;
            
            RegularSeed* mRegularSeed;
            ListTriangles* mListTriangles;
            GenTriangles* mGenTriangles;
            GenTrianglesDelta* mGenTrianglesDelta;
            
            TransitionSeed* mTransitionSeed;
            TransitionListTriangles* mTransitionListTriangles;
            TransitionGenTriangles* mTransitionGenTriangles;
            
            Ogre::MaterialPtr mDisplayMat;
            Ogre::MaterialPtr mDisplayDeltaMat;
            
            // Holds root level chunks
            ChunkMap mChunks;
            
            std::vector<Stopwatch*> mStopwatches;
            
            OctreeChunkManagerPolicy* mActivePolicy;
            OctreeChunkManagerPolicy* mDefaultPolicy;

        public:

            /**
             * @param camera The camera to use on rvs (maybe create a new camera in code)
            */
            OctreeChunkManager(Ogre::SceneManager* sceneMgr,
                               SharedParams* sharedParams,
                               Ogre::Camera* camera,
                               const Ogre::String& densityMaterialName,
                               const Ogre::String& displayMaterialName,
                               const Ogre::String& displayDeltaMaterialName);
            
            virtual ~OctreeChunkManager();
                        
            SharedParams* getSharedParams(){ return mSharedParams; }
            
            void setActivePolicy(OctreeChunkManagerPolicy* policy){ mActivePolicy = (policy == 0 ? mDefaultPolicy : policy); }
            
            /**
             * overload to customise the root chunks
            */
            virtual OctreeChunk* createRoot( Ogre::SceneNode* sceneNode, RenderVolumeWithTimeStamp* rvs );
            
            /**
             * creates chunks where all ProceduralRenderable's are null requires a parent OctreeChunk
            */
            OctreeChunk* createEmpty(const Int3& index, Ogre::uint8 lodLevel, OctreeChunk* parent=0, bool recursive=false);
            
            /**
             * @param recursive creates all child lod levels
            */
            OctreeChunk* create(const Int3& index, Ogre::uint8 lodLevel, OctreeChunk* parent=0, bool recursive=false);
            
            /**
             * Recreates the chunk, use if theres a density changed
            */
            bool recreate(OctreeChunk* chunk, bool recursive=false);
            
            /**
             * Creates a transition, which is a face of cells, that go from a low lod to a higher lod
            */
            void createTransition(OctreeChunk* chunk, Ogre::uint8 face, Ogre::uint8 side);

            /**
             * Recreates the transition, use if theres a density changed
             */
            void recreateTransition(OctreeChunk* chunk, Ogre::uint8 face, Ogre::uint8 side);
            
            /**
             * destroys the chunk all its children recursively
            */
            void destroy(OctreeChunk* chunk);
            
            /**
             * @return Null if no chunk foundm
            */
            OctreeChunk* getChunk(const Int3& index, const Ogre::uint8 lodLevel);
            
            /**
             * @return an iterator over all the "root" chunks (lod level 0)
            */
            ChunkIterator getChunkIterator();
            
            /**
             * logs the time data on running the various geometry shaders
            */
            void logStopwatches(Ogre::String& log);
            
            /**
             * logs the number of chunks at each lod level that are visible
            */
            void logLods(Ogre::String& log);

            GOOF::Int3 getQuadrant(Ogre::uint8 quadrantIndex)
            {
                return Int3( quadrantIndex & 1, quadrantIndex>>1 & 1, quadrantIndex>>2 & 1);
            }
            
            GOOF::Int3 getQuadrant(const GOOF::Int3& worldIndex)
            {
                return GOOF::Int3((unsigned int)worldIndex.x%2, (unsigned int)worldIndex.y%2, (unsigned int)worldIndex.z%2);
            }
            
            Ogre::uint8 getQuadrantIndex(const GOOF::Int3& quadrant)
            {
                return quadrant.x + quadrant.y*2 + quadrant.z*4;
            }
        
            const std::vector<Stopwatch*>& getStopwatchs() const
            {
                return mStopwatches;
            }
            
            /**
             * @return counts counts.length = num lod levels, element.first exists, element.second visible
            */
            void getLodCounts( std::vector< std::pair<Ogre::uint, Ogre::uint> >& counts );
            
        };
        
        
        class SimpleLodStratedy
        {
        private:
            SharedParams* mSharedParams;
            
            // size one greater than it strictly needs to be, split points ends with 0.0
            std::vector<Ogre::Real> mSplitPoints;
            
            // position that we last adjusted lods from
            Ogre::Vector3 mLastPos;
            
            bool mNeedsUpdate;

        public:
            SimpleLodStratedy(SharedParams* sharedParams);
            
            void needsUpdate() { mNeedsUpdate = true; }
            
            /** 
             * Copied from Ogre's PSSM shadow setup
             * @param lamdba lower value means more uniform, higher lambda means more logarithmic
            */
            void calculateSplitPoints(Ogre::Real nearDist, Ogre::Real farDist, Ogre::Real lambda = 0.95);
            
            /**
             * @return the calculated split points
            */
            const std::vector<Ogre::Real>& getSplitPoints() const { return mSplitPoints; }
            
            /**
             * If the difference between pos and SimpleLodStratedy::mLastPos is significant enough or SimpleLodStratedy::mNeedsUpdate is true
             * adjust the lod levels of chunks found in the chunk manager.
             * @param visualChange value set to true if a visual change occurs, if true should then call adjustLodTransitions()
             * @param proportion adjust lods if the distance between mLastPos and pos is greater than proportion * highest lod chunk size.
            */
            virtual void adjustLods(OctreeChunkManager* chunkMgr, const Ogre::Vector3& pos, bool& visualChange, const Ogre::Real& proportion = 0.1);
            
            /**
             * If a lod was changed then we need to call this to create and hide transition cells and adjust the delta shifts for regular cells
            */
            void adjustLodTransitions(OctreeChunkManager* chunkMgr);
            
        protected:
            
            /**
             * continue to try and generate child chunks?
            */
            virtual bool continueBranch(OctreeChunk* chunk, const Ogre::Vector3& pos);
            
            /**
             * Changes the visibility on chunks based on split points.
             * @param lodChange has a chunks lod been changed.
            */
            void adjustLod(const Ogre::Vector3& pos, OctreeChunkManager* chunkMgr, OctreeChunk* chunk, bool& visualChange);

            /**
             * If a lod was changed then we need to create and hide transition cells and adjust the delta shifts for regular cells
            */
            void adjustLodTransitions(OctreeChunkManager* chunkMgr, OctreeChunk* chunk);
        };
        
        
        /**
         * Gather stats of poly count at each lod level, mean, sigma, for a chunk with poly count > mean + sigma increase lod level
         * and for a chunk with poly count < mean - sigma 'could' decrease lod level.
        */
        class SmartLodStratedy : public SimpleLodStratedy
        {
        private:
            struct PolyStats
            {
                size_t sum;
                size_t n;
                float mean;
                float sigma;
                float sum_xi_minus_mean_sq;
            };
            
            std::vector< PolyStats > mLodPolyStats;
            
            int mPassedSplitIdx;
            
        public:
            SmartLodStratedy(SharedParams* sharedParams);
            
            virtual void adjustLods(OctreeChunkManager* chunkMgr, const Ogre::Vector3& pos, bool& visualChange, const Ogre::Real& proportion = 0.1);
            
        protected:
            /**
             * continue to try and generate child chunks?
             */
            virtual bool continueBranch(OctreeChunk* chunk, const Ogre::Vector3& pos);
        };

        
        /**
         * stores a zero level chunk
        */
        class OctreeChunkSelector
        {
        private:
            VT::OctreeChunk* mSelected;
            
        public:
            OctreeChunkSelector();
            virtual ~OctreeChunkSelector();
            
            /**
             * use null to unselect
            */
            void select( VT::OctreeChunk* chunk );
            
            /**
             * @return the currently selected
            */
            VT::OctreeChunk* getSelected ( ) { return mSelected; }
        };

    }
}
#endif
