#include "GOOFVTNibbler.h"
#include "GOOFVT.h"
#include "GOOFRenderTexture.h"
#include "GOOFSharedData.h"
#include "GOOFStopwatch.h"

namespace GOOF
{
    namespace VT
    {
        Nibbler::Nibbler(const SharedParams* params, Ogre::Real magnitude, const Ogre::Vector3& attenuation, const Ogre::String& materialName)
        {
            mNibblerMat = Ogre::MaterialManager::getSingleton().load(materialName, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME).staticCast<Ogre::Material>();
            setMagnitudeAndAttenuation(magnitude, attenuation);
        }
        
        Ogre::Real Nibbler::computeRangeFromAttenuation(Ogre::Real c, Ogre::Real b, Ogre::Real a, Ogre::Real magnitude)
        {
            // ax^2 + bx + c = 0;
            // x = (-b +- sqrt(b^2 - 4ac))/2a
            
            int threshold_level = 5; // For DS difference of 10-15 levels deemed unnoticeable
            float threshold = 1.0f/((float)threshold_level/(256.0f*magnitude));
            
            // from DLight::setAttenuation
            if(c != 1.0f || b != 0.0f || a != 0.0f)
            {
                // Use quadratic formula to determine outer radius
                c = c-threshold;
                float d = sqrt(b*b-4*a*c);
                float outerRadius = (-2*c)/(b+d);
                
                return outerRadius * 1.2;
            }
            else if(c == 1)
            {
                return 0.0;
            }
            else if(b == 0)
            {
                return sqrt(Ogre::Math::Abs(-(c-threshold)/a));
            }
            else
            {
                return Ogre::Math::Abs(-(c-threshold)/b);
            }
        }

        void Nibbler::setMagnitudeAndAttenuation(Ogre::Real magnitude, const Ogre::Vector3& attenuation)
        {
            mMagnitude = magnitude;
            mNibblerMat->getTechnique(0)->getPass(0)->getFragmentProgramParameters()->setNamedConstant("g_nibbler_magnitude", magnitude);

            Ogre::Real attenuationRange = computeRangeFromAttenuation(attenuation.x, attenuation.y, attenuation.z, mMagnitude);
            mAttenuation = Ogre::Vector4(attenuationRange, attenuation.x, attenuation.y, attenuation.z);
            
            mNibblerMat->getTechnique(0)->getPass(0)->getFragmentProgramParameters()->setNamedConstant("g_nibbler_attenuation", mAttenuation);
        }
        
        void Nibbler::setMagnitude(Ogre::Real magnitude)
        {
            mMagnitude = magnitude;
            mNibblerMat->getTechnique(0)->getPass(0)->getFragmentProgramParameters()->setNamedConstant("g_nibbler_magnitude", magnitude);
            
            Ogre::Real attenuationRange = computeRangeFromAttenuation(mAttenuation.x, mAttenuation.y, mAttenuation.z, mMagnitude);
            mAttenuation.x = attenuationRange;
            
            mNibblerMat->getTechnique(0)->getPass(0)->getFragmentProgramParameters()->setNamedConstant("g_nibbler_attenuation", mAttenuation);
        }

        void Nibbler::setNibbles(bool nibbles)
        {
            Ogre::SceneBlendOperation sbo = nibbles == true ? Ogre::SBO_REVERSE_SUBTRACT  : Ogre::SBO_ADD;
            mNibblerMat->getTechnique(0)->getPass(0)->setSceneBlendingOperation(sbo);
        }
        
        void Nibbler::update(Ogre::SceneManager* sceneMgr, RenderVolume* rvs, const Int3& index, const Ogre::Vector3& worldPos, const SharedParams* params)
        {            
            Ogre::Vector3 texPos = params->toTextureSpace( index, worldPos );
            
            Ogre::Real attenuationRange = mAttenuation.x;
            Ogre::Vector3 texScale = params->getWorldSpaceToTextureSpaceScale() * attenuationRange;
            
            Ogre::Vector3 min = index.toVector3() * params->getLodChunkSize(0);
            
            Ogre::Pass* pass = mNibblerMat->getTechnique(0)->getPass(0);
            pass->getFragmentProgramParameters()->setNamedConstant("g_nibbler_position", worldPos - min);
            
            // debug cover full volume
            //texPos = Ogre::Vector3(0.5, 0.5, 0.5);
            //texScale = Ogre::Vector3(1.0, 1.0, 1.0);
            
            //update(sceneMgr, rvs, texPos, texScale);
            
            rvs->renderSection(sceneMgr, pass, texPos, texScale);
        }
        
        void Nibbler::update(Ogre::SceneManager* sceneMgr, RenderVolume* rvs, const Int3& index, const Ogre::Vector3& worldPos, const SharedParams* params, Stopwatch* stopwatch)
        {
            stopwatch->start();
            update(sceneMgr, rvs, index, worldPos, params);
            stopwatch->stop();
        }
        
    }
}
