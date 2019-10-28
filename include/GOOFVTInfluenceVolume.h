#ifndef __GOOFVTInfluenceVolume_H
#define __GOOFVTInfluenceVolume_H

#include "GOOFInt3.h"
#include <Ogre.h>

#include "GOOFVTOctreeChunk.h"

namespace GOOF 
{
    class RenderVolume;
    
    namespace VT
    {
        class SharedParams;
        
        /**
         * A single point in a render volume.
        */
        class InfluencePoint
        {
        public:
            Ogre::Vector3 worldPos;
            Ogre::Real value; 
        };
        
        
        class RootOctreeChunkPlusInfluence : public RootOctreeChunk
        {
        public:
            std::vector<InfluencePoint> mInfluencePoints;
            
            // holds influence volume
            RenderVolume* mInfluence;
            
        public:
            RootOctreeChunkPlusInfluence() : RootOctreeChunk() {}
            virtual ~RootOctreeChunkPlusInfluence() {}
        };
        
        
        class OctreeChunkManagerPlusInfluence : public OctreeChunkManager
        {
        protected:
            // dimensions of render volume (cube)
            Ogre::uint mInfluenceVolumeDim;
            
        public:
            OctreeChunkManagerPlusInfluence(Ogre::SceneManager* sceneMgr,
                                            SharedParams* sharedParams,
                                            Ogre::Camera* camera,
                                            const Ogre::String& densityMaterialName,
                                            const Ogre::String& displayMaterialName,
                                            const Ogre::String& displayDeltaMaterialName,
                                            const Ogre::uint& influenceVolumeDim = 64);
            
            RenderVolume* createInfluenceVolume();
            
            /**
             * @return new RootOctreeChunkPlusInfluence
            */
            virtual OctreeChunk* createRoot( Ogre::SceneNode* sceneNode, RenderVolumeWithTimeStamp* rvs );
        };

        
        /**
         * 'directed' texturing.
         * 'write' shader sets the influence of a selection of individual pixels of the render volume.
         * 'propagate' shader operates over the full volume and does the influence propagation.
         *
         * Method is to write() for all volumes, then for each volume do a single propagation step,
         * each volume has links for potentially 6 sides, render volumes top, bottom, front, back, left, right.
         * Iterate the propagation step.
         *
         * Note this is approx. as ignoring the edges and corners we truelly need 26 surrounding render volumes.
        */
        class InfluenceVolume
        {
        private:
            //bool mPreserve;
            Ogre::Real mDecay;
            Ogre::Real mMomentum;
            Ogre::MaterialPtr mWriteMat;
            Ogre::MaterialPtr mPropagateMat;
            
            // stores the results of a propagation step
            RenderVolume* mResult;

        public:
            InfluenceVolume( RenderVolume* result,
                             const Ogre::String& writeMaterialName = "VT/InfluenceWrite",
                             const Ogre::String& propagateMaterialName = "VT/InfluencePropagate");
            
            /**
             * equivalent to SharedParams::toTextureSpace but influence volume has no margins
             * @param index The index of a root chunk
             * @return The world space position converted to texture space
             */
            static Ogre::Vector3 toTextureSpace( const Int3& index, const Ogre::Vector3& worldPos, const SharedParams* params );
            
            /**
             * writes influence points to the volume.
            */
            void write( Ogre::SceneManager* sceneMgr, RenderVolume* rvs, const Int3& index, const std::vector<InfluencePoint>& points, const SharedParams* params );
            
            //void setPreserve( const bool preserve);
            //bool getPreserve() const { return mPreserve; }
            
            /**
             * effects how propagation happens (see shaders)
            */
            void setDecay( const Ogre::Real decay );
            Ogre::Real getDecay() const { return mDecay; }
            
            /**
             * effects how fast propagation happens (see shaders)
            */
            void setMomentum( const Ogre::Real momentum );
            Ogre::Real getMomentum() const { return mMomentum; }

            /**
             * propagates the influence, requires a second or result render volume to bounce see constructor
            */
            void propagate( Ogre::SceneManager* sceneMgr, RenderVolume*& rvs );
        };
    }
}
#endif
