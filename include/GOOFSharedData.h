#ifndef __GOOFSharedData_H
#define __GOOFSharedData_H

#include "GOOFPrerequisites.h"
#include <OgreSingleton.h>
#include <atomic>

namespace GOOF 
{
	class SharedData : public Ogre::Singleton<SharedData> 
	{
	public:
		SharedData();
        ~SharedData();
        
        Ogre::Root*             mRoot;
		Ogre::SceneManager*		mSceneMgr;
		Ogre::Camera*			mCamera;
		Ogre::RenderWindow*		mWindow;
		Ogre::Viewport*			mViewport;
		std::atomic<bool>       mQuit;
		std::map<Ogre::String, Ogre::ShadowCameraSetupPtr> mShadowCameraSetups;
        
        // Full screen rectangle for RTT
        Ogre::Rectangle2D* mRect;
        
        DS::System* mDSSystem;
        
		SequenceInputsStateSet* mSequenceInputsStateSet;
        
		ActionSet* mActionSet;
        
        // For camera systems etc
		Ogre::UserObjectBindings mUserObjectBindings;
        
        // Names of camera systems for lookup into mUserObjectBindings
        Ogre::StringVector mCameraSystemNames;        
        
    public:
        static SharedData* getSingletonPtr();
		static SharedData& getSingleton();
                
        Ogre::ShadowCameraSetupPtr getShadowCameraSetup (const Ogre::String& name);
		Ogre::String getShadowCameraSetupName (const Ogre::ShadowCameraSetupPtr& cameraSetup);
        
        // Full list of all lights in the scene
        Ogre::LightList getLightList();
        
		void quit() { mQuit = true; }
    };
    
    /**
     * Derived from this to simplify access to SharedData
    */
    class SharedDataAccessor
	{
	public:
		static Ogre::SceneManager* getSceneManager() { return SharedData::getSingleton().mSceneMgr; }
		static Ogre::Camera* getCamera()  { return SharedData::getSingleton().mCamera; }
		static Ogre::RenderWindow* getRenderWindow() { return SharedData::getSingleton().mWindow; }
		static Ogre::Viewport* getViewport() { return SharedData::getSingleton().mViewport; }
        static Ogre::Rectangle2D* getRect() { return SharedData::getSingleton().mRect; }
		static DS::System* getDSSystem() { return SharedData::getSingleton().mDSSystem; }
		static Ogre::UserObjectBindings& getUserObjectBindings() { return SharedData::getSingleton().mUserObjectBindings; }
		static std::map<Ogre::String, Ogre::ShadowCameraSetupPtr>& getShadowCameraSetupMap() { return SharedData::getSingleton().mShadowCameraSetups; }
		static Ogre::ShadowCameraSetupPtr getShadowCameraSetup(const Ogre::String& name) { return SharedData::getSingleton().getShadowCameraSetup(name); }
    };
    
}
#endif
