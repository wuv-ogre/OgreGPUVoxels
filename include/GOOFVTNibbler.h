#ifndef __GOOFVTNibbler_H
#define __GOOFVTNibbler_H

#include "GOOFInt3.h"
#include <Ogre.h>

namespace GOOF 
{
    class RenderVolume;
    class Stopwatch;
    
    namespace VT
    {
        class SharedParams;
        
        /**
         * Nibbler adds or removes spherical "soft lumps" to or from the density volume
         * Modifies density through rendering a Rectangle2D primative with its scale adjusted at the required slices to 
         * minimise the number of pixels rendered
        */
        class Nibbler
        {
        public:
            /**
             * @param attenuation range, constant, linear, quadratic
             */
            Nibbler(const SharedParams* params, Ogre::Real magnitude = 10.0, const Ogre::Vector3& attenuation = Ogre::Vector3(1.0, 0.35, 0.44), const Ogre::String& materialName = "VT/Nibbler");
            
            /**
             * @param magnitude This value * attenuation gets added to or subrtacted from density function, values changes the attenuation range
             * @param attenuation constant, linear, quadratic
            */
            void setMagnitudeAndAttenuation(Ogre::Real magnitude, const Ogre::Vector3& attenuation);
            
            /**
             * @param magnitude This value * attenuation gets added to or subrtacted from density function, values changes the attenuation range
            */
            void setMagnitude(Ogre::Real magnitude);
            
            /**
             * @return the magnitude of the nibbler
            */
            Ogre::Real getMagnitude(){ return mMagnitude; }
            
            /**
             * @return the range, constant, linear, quadratic
            */
            Ogre::Vector4 getAttenuation(){ return mAttenuation; }
            
            /**
             * Either nibbler the denisty or add to it by changing the scene_blend_op
            */
            void setNibbles(bool nibbles);
            
            /*
             * Updates the rvs to either subtract or add a "soft lump"
             * @param index the index of the chunk
             * @param worldPos the position of the center of the nibbler in world space
            */
            void update(Ogre::SceneManager* sceneMgr, RenderVolume* rvs, const Int3& index, const Ogre::Vector3& worldPos, const SharedParams* params);
            void update(Ogre::SceneManager* sceneMgr, RenderVolume* rvs, const Int3& index, const Ogre::Vector3& worldPos, const SharedParams* params, Stopwatch* stopwatch);
            
        private:
            Ogre::Real computeRangeFromAttenuation(Ogre::Real constant, Ogre::Real linear, Ogre::Real quadratic, Ogre::Real magnitude);
            
            Ogre::MaterialPtr mNibblerMat;            
            Ogre::Vector4 mAttenuation;
            Ogre::Real mMagnitude;
        };
    }
    
    /**
     * Takes an input position finds the nearest voxel coord then we adjust the density volume so that we either add or subtract a cube
    */
    /*
    class PixiNibbler
    {
    };
    */
}
#endif
