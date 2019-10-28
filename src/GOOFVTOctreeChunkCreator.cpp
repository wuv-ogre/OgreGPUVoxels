#include "GOOFVTOctreeChunkCreator.h"
#include "OgreRoot.h"

namespace GOOF
{
namespace VT
{
	 Ogre::SceneNode* OctreeChunkCreator::getSceneNode() 
	 { 
		 return getSceneManager()->getRootSceneNode(); 
	 }
}
}