#include "GOOFSharedData.h"
#include <OgreRectangle2D.h>
#include <OgreLogManager.h>

namespace Ogre
{
    template<> GOOF::SharedData* Ogre::Singleton<GOOF::SharedData>::msSingleton = 0;
}

namespace GOOF
{
    SharedData::SharedData() :
    mSceneMgr(0),
    mCamera(0),
    mWindow(0),
    mViewport(0),
    mQuit(false),
    mDSSystem(0),
    mSequenceInputsStateSet(0),
    mActionSet(0)
    {
        mRect = new Ogre::Rectangle2D(true);
        mRect->setCorners(-1,1,1,-1);
    }
    SharedData::~SharedData()
    {
        if(mRect)
            delete mRect;
        
        mShadowCameraSetups.clear();
    }

    
    SharedData* SharedData::getSingletonPtr(void)
	{
		return msSingleton;
	}
	
	SharedData& SharedData::getSingleton(void)
	{
		assert( msSingleton );  return ( *msSingleton );
	}
    
    Ogre::ShadowCameraSetupPtr SharedData::getShadowCameraSetup (const Ogre::String& name)
    {
        std::map<Ogre::String, Ogre::ShadowCameraSetupPtr>::iterator it = mShadowCameraSetups.find(name);
        
        if(it != mShadowCameraSetups.end())
        {
            return it->second;
        }
        else
        {
            Ogre::LogManager::getSingleton().logMessage("SharedData::getShadowCameraSetup no ShadowCameraSetup named " + name);
            assert(false);
        }
    }
    
    Ogre::String SharedData::getShadowCameraSetupName (const Ogre::ShadowCameraSetupPtr& cameraSetup)
    {
        std::map<Ogre::String, Ogre::ShadowCameraSetupPtr>::iterator it_end = mShadowCameraSetups.end();
        for(std::map<Ogre::String, Ogre::ShadowCameraSetupPtr>::iterator it =  mShadowCameraSetups.begin(); it != it_end; ++it)
        {
            if(it->second.getPointer() == cameraSetup.getPointer())
            {
                return it->first;
            }
        }
        
        Ogre::LogManager::getSingleton().logMessage("SharedData::getShadowCameraSetupName no matching ShadowCameraSetup in map");
        assert(false);
    }
    
    Ogre::LightList SharedData::getLightList()
    {
        Ogre::LightList lights;
        Ogre::SceneManager::MovableObjectIterator i =
        mSceneMgr->getMovableObjectIterator("Light");
        for (; i.hasMoreElements(); i.moveNext())
        {
            assert(i.peekNextValue()->getMovableType() == "Light");
            Ogre::Light *light = static_cast<Ogre::Light*>(i.peekNextValue());
            lights.push_back(light);
        }
        return lights;
    }
}
