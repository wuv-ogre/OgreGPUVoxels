#include "GOOFVTOctreeChunkCreatorWithEdit.h"
#include "GOOFVTRoot.h"
#include "GOOFSlicing.h"
#include "GOOFVTDensity.h"

using namespace Ogre;

namespace GOOF
{
namespace VT
{
	OctreeChunkCreatorWithEdit::OctreeChunkCreatorWithEdit() 
	{
		mModified_rvs = NULL; 
		mNibblerMat[0] = Ogre::MaterialManager::getSingleton().load("VT/DensityNibbler/Subtract", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME).staticCast<Material>();
		mNibblerMat[1] = Ogre::MaterialManager::getSingleton().load("VT/DensityNibbler/Add", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME).staticCast<Material>();
	}

	void OctreeChunkCreatorWithEdit::modify(OctreeChunk* chunk, const Ogre::Vector3& pos, const Ogre::Vector4& attenuation, Ogre::Real weight, bool add)
    {
		Ogre::Vector3 range(attenuation.x, attenuation.x, attenuation.x);
        
        Root* root = Root::getSingletonPtr();
        Ogre::Vector3 chunkSize = root->getLodChunkSize(0);
        Ogre::Vector3 voxelSize = root->getLodVoxelSize(0);
		Ogre::Vector3 voxelSizeHighestLod = root->getLodVoxelSize(root->getLodCount() - 1);
		GOOF::Int3 index = chunk->getIndex();

        Ogre::Vector3 wsVolLower = index.toVector3() * chunkSize - root->getMargin() * voxelSize;
        Int3 lower, upper;
		Ogre::Vector3 temp;

		temp = (pos - range - wsVolLower) / voxelSizeHighestLod;
		for(int i=0; i<3; i++)
			temp[i] = Ogre::Math::Floor(temp[i]);
        lower.fromVector3(temp);

		temp = (pos + range - wsVolLower) / voxelSizeHighestLod;
		for(int i=0; i<3; i++)
			temp[i] = Ogre::Math::Ceil(temp[i]);
		upper.fromVector3(temp);
        
		for(int i=0; i<3; i++)
		{
			if(lower[i] < 0)
				lower[i] = 0;
			if(lower[i] > root->getRenderVolumeSize()-1)
				lower[i] = root->getRenderVolumeSize()-1;
			if(upper[i] < 0)
				upper[i] = 0;
			if(upper[i] > root->getRenderVolumeSize()-1)
				upper[i] = root->getRenderVolumeSize()-1;
		}

		assert(chunk->rvs);
        Ogre::TexturePtr volume = chunk->rvs->getRenderVolume();

		Ogre::MaterialPtr mat = mNibblerMat[(int) add];
	
		Ogre::GpuSharedParametersPtr sharedParams = root->getGpuSharedParameter();
		sharedParams->setNamedConstant("world_space_chunk_pos", chunk->getIndex().toVector3() * chunkSize);
		sharedParams->setNamedConstant("world_space_chunk_size", chunkSize);

		const Ogre::GpuProgramParametersSharedPtr& params = mat->getTechnique(0)->getPass(0)->getFragmentProgramParameters();
		params->setNamedConstant("nibbler_position", pos);
		params->setNamedConstant("nibbler_attenuation", attenuation);
		params->setNamedConstant("nibbler_magnitude", weight);

		Ogre::GpuProgramParametersSharedPtr vparams = mat->getTechnique(0)->getPass(0)->getVertexProgramParameters();
		chunk->rvs->render(getSceneManager(), mat, vparams);

		mModified_rvs = chunk->rvs;

		destroy(chunk, true);
		createRecursive(index);
    }

	Slicing* OctreeChunkCreatorWithEdit::createOrRetrieveSlicingVolume(OctreeChunk* parent, bool rvsStore)
	{
		Slicing* result;

		if(mModified_rvs)
		{
			result = mSlicingToRecycle.back();
			assert(result == mModified_rvs);
			mSlicingToRecycle.pop_back();
			mModified_rvs = NULL;
		}
		else
		{
			result = OctreeChunkCreatorM2::createOrRetrieveSlicingVolume(parent, rvsStore);
		}

		return result;
	}


} // ends namespace VT
} // ends namespace GOOF
