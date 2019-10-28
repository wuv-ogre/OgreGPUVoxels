#ifndef GOOFVTApplication2_H
#define GOOFVTApplication2_H

#define WITH_OGRE_OVERLAY_SYSTEM

#include <Ogre.h>
#include <OgreOverlaySystem.h>
#include <CEGUI/ForwardRefs.h>
#include "GOOFEditorBuildUnit.h"
#include "GOOFEditorWindow.h"
#include "GOOFFrameInfo.h"
#include "GOOFDSRendering.h"
#include "GOOFShadowParams.h"
#include "GOOFApplicationInstance.h"
#include "GOOFSceneQuery2Type.h"

namespace GOOF
{
    class Int3;
    
    namespace VT
    {
        class SharedParams;
        class OctreeChunkManager;
        class OctreeChunkManagerSpherePolicy;
        class InfluenceVolume;
        class RayCast;
        class OctreeChunk;
        class Nibbler;
        class SimpleLodStratedy;
        class OctreeChunkSelector;
        typedef SceneQuery2Type<Ogre::RaySceneQuery, SceneQueryListener2Type<OctreeChunk, Ogre::RaySceneQueryListener> > RaySceneQueryOctreeChunk;
    }

    typedef EditorUnit < Loki::GenLinearHierarchy < LOKI_TYPELIST_3(ShadowParams, DSRendering, FrameInfo), EditorBuildUnit, EditorWindow > > MyEditor;
    typedef Loki::GenLinearHierarchy < LOKI_TYPELIST_3(MyEditor, DSUnit, DebugTextureManagerUnit), ApplicationBuildUnit, ApplicationBase > ApplicationHierarchy;
    
    class VTApplication2 : public ApplicationHierarchy
    {
    private:
        VT::OctreeChunkManager* mOctreeChunkManager;
        VT::OctreeChunkManagerSpherePolicy* mOctreeChunkManagerSpherePolicy; // for nibbler
        VT::InfluenceVolume* mInfluenceVolume;
        
        VT::RaySceneQueryOctreeChunk mQuery;
        VT::RayCast* mRaycast;
        VT::Nibbler* mNibbler;
        VT::SimpleLodStratedy* mLodStratedy;
        VT::OctreeChunkSelector* mOctreeChunkSelector;
        
        Ogre::SceneNode* mDebugNode;
        Ogre::uint8 mVisibleLodLevel;
        
        Ogre::SceneNode* mDirLightNode;
        bool mPauseLightAnimation;
        
        Ogre::String mDenistyMatName;
        Ogre::String mDisplayMatName;
        Ogre::String mDisplayDeltaMatName;
        
        CEGUI::ToggleButton* mAdjustLodsToggleButton;
        CEGUI::Window* mSelectInfo;
        //CEGUI::Window* mModifiedInfo;
        
        CEGUI::ToggleButton* mPlaceInfluenceToggleButton;
        CEGUI::Slider* mInfluenceValueSlider;
        CEGUI::Spinner* mPropagationSpinner;
        
        // time data on running the various geometry shaders
        CEGUI::MultiColumnList* mStopwatchDataMultiColumnList;
        CEGUI::MultiColumnList* mLodDataMultiColumnList;
        
    public:
        VTApplication2();
        virtual ~VTApplication2();
        
        virtual const Ogre::String& getResourcesFilename() const { static Ogre::String filename = "resourcesGOOF.cfg"; return filename; }
                
        virtual void addResourceLocations();
        virtual void setupShadows();
        virtual bool setupFinish();
        
        virtual void update(const float deltaTime);
        virtual void keyPressed(const OIS::KeyEvent &arg);
        
        void testGeometryShaders();
        void testRegular(VT::SharedParams* sharedParams);
        void testRegularBatch(VT::SharedParams* sharedParams);
        void testTransition(VT::SharedParams* sharedParams);
        void testDensityArrayVsVolumeSpeed(VT::SharedParams* sharedParams);
        void testRenderVolumeSubsectionRender(VT::SharedParams* sharedParams);
        void testRenderObjectToVolume();
        void testLodPointGeneration(VT::SharedParams* sharedParams);
        void testInfluenceVolume(VT::SharedParams* sharedParams);

        /**
         * aab then polygon level
        */
        VT::OctreeChunk* findNearest( const Ogre::Ray& ray, Ogre::Real& dist );

        /**
         * utility for octree manager, create a grid of chunks
        */
        void createOctreeChunks(const Int3& min, const Int3& max);
        
        /**
         * change the default starting density function to use the one for craftscape
        */
        void changeDensityToCraftscape();
        
        /**
         * when not using a lod stratedy, manually set which lod to view
        */
        void setChunkVisible(VT::OctreeChunk* chunk, Ogre::uint8 visibleLodLevel);

        /**
         * for debugging
        */
        //void showChunkRenderVolumes(VT::OctreeChunk* chunk);

        
        //void doRayCast(VT::OctreeChunk* chunk, Ogre::uint8 lodlevel, Ogre::Ray& ray, Ogre::Real& nearest);
        
        void recreateVisibleChunksWithin(VT::OctreeChunk* chunk, VT::OctreeChunk* parent, const Int3& idx, Ogre::uint8 lodLevel, const Ogre::Vector3& pt, const Ogre::Real& radius);
        
    };
}
#endif
