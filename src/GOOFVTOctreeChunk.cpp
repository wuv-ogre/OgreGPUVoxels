#include "GOOFVTOctreeChunk.h"
#include "GOOFVTOctreeChunkCreator.h"
#include "GOOFCameraUtils.h"

namespace GOOF
{
namespace VT
{
	//  x*1 + y*2 + z*4 = index
	const GOOF::Int3 OctreeChunk::msQuadrant[8] = 
	{
		GOOF::Int3(0,0,0),
		GOOF::Int3(1,0,0),
		GOOF::Int3(0,1,0),
		GOOF::Int3(1,1,0),
		GOOF::Int3(0,0,1),
		GOOF::Int3(1,0,1),
		GOOF::Int3(0,1,1),
		GOOF::Int3(1,1,1)
	};

	//  (i%2 == 1) ? i -1 : i + 1 = reverse index
	const GOOF::Int3 OctreeChunk::msFace[6] = 
	{
		GOOF::Int3::UNIT_X,
		GOOF::Int3::NEGATIVE_UNIT_X,
		GOOF::Int3::UNIT_Y,
		GOOF::Int3::NEGATIVE_UNIT_Y,
		GOOF::Int3::UNIT_Z,
		GOOF::Int3::NEGATIVE_UNIT_Z
	};
	const Ogre::Vector3 OctreeChunk::msTransitionFace[3] = 
	{
		Ogre::Vector3::UNIT_X,
		Ogre::Vector3::UNIT_Y,
		Ogre::Vector3::UNIT_Z
	};
	const Ogre::Vector3 OctreeChunk::msOneMinusTransitionFace[3] = 
	{
		Ogre::Vector3::UNIT_SCALE - Ogre::Vector3::UNIT_X,		
		Ogre::Vector3::UNIT_SCALE - Ogre::Vector3::UNIT_Y,
		Ogre::Vector3::UNIT_SCALE - Ogre::Vector3::UNIT_Z
	};

	OctreeChunk::OctreeChunk(OctreeChunkCreator* creator, const GOOF::Int3& index, Ogre::uint8 lodLevel) : 
		mCreator(creator), 
			mIndex(index), 
			mParent(0), 
			mLodLevel(lodLevel),
			rvs(0)
	{
		for(int i=0; i<8; i++)
		{
			mChildren[i] = NULL;
			mChildNeedsCreation[i] = true;
		}
		for(int i=0; i<6; i++)
		{
			mTransition[i] = NULL;
			mSibling[i] = NULL;
			mTransitionNeedsCreation[i] = true;
		}
	}

	
	OctreeChunk::~OctreeChunk()
	{
		getParentSceneNode()->detachObject(this);

		for(int i=0; i<6; i++)
		{
			if(mSibling[i])
			{
				int reverse = (i%2 == 1) ? i -1 : i + 1;
				mSibling[i]->mSibling[reverse] = NULL;
			}
			if(mTransition[i])
			{
				mTransition[i]->getParentSceneNode()->detachObject(mTransition[i]);
				delete mTransition[i];
			}
		}		
		for(int i=0; i<8; i++)
		{
			if(mChildren[i] != NULL)
				delete mChildren[i];
		}
	}

	void OctreeChunk::setVisible (bool visible)
	{
		if(visible != isVisible())
		{
			ProceduralRenderable::setVisible(visible);
			if(!visible)
			{
				for(int i=0; i<6; i++)
				{
					if(mTransition[i])
					{
						mTransition[i]->setVisible(visible);
					}
				}
			}
		}
	}

	void OctreeChunk::addChild(OctreeChunk* child, int index)
	{
		assert(child);
		mChildren[index] = child;
		child->mParent = this;
	}

	void OctreeChunk::setVisibleLod(const Ogre::uint8 lodLevel, Ogre::uint8 depth)
	{
		if(lodLevel == depth)
			setVisible(true);
		else
			setVisible(false);

		for(int i=0; i<8; i++)
		{
			if(mChildren[i])
			{
				mChildren[i]->setVisibleLod(lodLevel, depth + 1);
			}
		}
	}

	void OctreeChunk::hideAllChildren()
	{
		for(int i=0; i<8; i++)
		{
			if(mChildren[i])
			{
				mChildren[i]->setVisible(false);
				mChildren[i]->hideAllChildren();
			}
		}
	}

	OctreeChunk* OctreeChunk::createOrRetrieveChild(Ogre::uint8 i)
	{
		if(mChildNeedsCreation[i])
		{	
			mCreator->create(getIndex() * 2 + OctreeChunk::msQuadrant[i], this, false, i);
		}
	
		return mChildren[i];
	}

	ProceduralRenderable* OctreeChunk::createOrRetrieveTransition(Ogre::uint8 i)
	{
		if(mTransitionNeedsCreation[i])
		{
			Ogre::uint8 side = i%2;
			Ogre::uint8 face = (i - side)/2;
			mCreator->createTransition(this, face, side);
		}

		return mTransition[i];
	}
	
    void OctreeChunk::adjustTransitions(Ogre::uint8 lodLimit)
	{
		if(mLodLevel <= lodLimit)
        {
			if(isVisible())
			{
				for(int side=0; side<2; side++)
				{
					Ogre::Vector4 deltaFaces(0,0,0,0);	

					for(int face=0; face<3; face++)
					{
						Ogre::uint8 i = face*2 + side;	// transition index
						if(mTransition[i])
						{
							// different lod between siblings 
							// and transition is from lower lod to higher lod
							if(mSibling[i] && !mSibling[i]->isVisible() && 
							  (!mSibling[i]->mParent || !mSibling[i]->mParent->isVisible()) )
							{
								deltaFaces[face] = 1;
								mTransition[i]->setVisible(true);
							}
							else
							{
								mTransition[i]->setVisible(false);
							}
						}
					}

					// 0 is the positive face
					setCustomParameter(side, deltaFaces);
				}
			}
			else
			{
				for(int i=0; i<8; i++)
				{
					if(mChildren[i])
					{
						mChildren[i]->adjustTransitions(lodLimit);
					}
				}
			}
		}
	}

}
}