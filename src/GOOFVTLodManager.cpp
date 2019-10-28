#include "GOOFVTLodManager.h"
#include "GOOFVTRoot.h"
#include "GOOFVTOctreeChunk.h"
#include "GOOFProceduralRenderable.h"

using namespace Ogre;

namespace GOOF
{
namespace VT
{
	LodManager::LodManager(Ogre::Real viewShift) :
		mViewShiftSquared(viewShift * viewShift),
        mLastScopePosition(Ogre::Math::POS_INFINITY)
	{
		mLodCamera.resize(VT::Root::getSingleton().getLodCount()*2);
	}

	LodManager::~LodManager()
	{
		for(ChunkMap::iterator it = mChunks.begin(); it != mChunks.end(); ++it)
		{
			destroy(it->second);
		}
		for(std::vector<Ogre::Camera*>::iterator it = mLodCamera.begin(); it != mLodCamera.end(); ++it)
		{
			getSceneManager()->destroyCamera(*it);
		}
		mChunks.clear();
	}

	void LodManager::setViewPositionAndOrientation(const Ogre::Vector3 pos, const Ogre::Quaternion& ori)
	{
		for(int lodLevel=0; lodLevel< VT::Root::getSingleton().getLodCount()*2; lodLevel++)
		{
			mLodCamera[lodLevel]->setPosition(pos);
			mLodCamera[lodLevel]->setOrientation(ori);
		}
	}

	void LodManager::setLodSplit(Ogre::Camera* syncCamera, const Ogre::Real splitDepth, const Ogre::Real crossOver, const Ogre::uint8 lodLevel)
	{
		assert(lodLevel < VT::Root::getSingleton().getLodCount());

		for(int i=0; i<2; i++)
		{
			Ogre::Camera* lodCamera = mLodCamera[lodLevel];
			if(!lodCamera)
			{
				lodCamera = getSceneManager()->createCamera("VT/LodCamera/" + Ogre::StringConverter::toString(lodLevel) + "/" + Ogre::StringConverter::toString(i));
				//mViewNode->attachObject(lodCamera);
			}

			//lodCamera->synchroniseBaseSettingsWith(syncCamera);
			
			if(i == 1)
			{
				lodCamera->setFarClipDistance(splitDepth+crossOver);
			}
			else
			{
				lodCamera->setFarClipDistance(splitDepth);
			}

			lodCamera->setNearClipDistance(0.0001);

			lodCamera->setProjectionType(syncCamera->getProjectionType());
			lodCamera->setFOVy(syncCamera->getFOVy());
			lodCamera->setAspectRatio(syncCamera->getAspectRatio());
			lodCamera->setFocalLength(syncCamera->getFocalLength());
			lodCamera->setPosition(Ogre::Vector3::ZERO);
			lodCamera->setOrientation(Ogre::Quaternion::IDENTITY);

			mLodCamera[lodLevel*2 + i] = lodCamera;
		}
	}


	bool LodManager::isVisible(OctreeChunk* chunk, const Ogre::uint8 lodLevel)
	{
		// to prevent flickering between lod levels the padded version is currently  +1 voxel dim to min and max of aab
		if(chunk->isVisible())
		{
			return mLodCamera[lodLevel*2+1]->isVisible(chunk->getBoundingBox());

			//return mLodCamera[lodLevel]->isVisible(chunk->getWorldBoundingBoxPadded());
		}
		else
		{
			return mLodCamera[lodLevel*2]->isVisible(chunk->getBoundingBox());
			//return mLodCamera[lodLevel]->isVisible(chunk->getWorldBoundingBox());
		}
	}

	OctreeChunk* LodManager::getChunk(const Int3& cell, const Ogre::uint8 lodLevel)
	{
		OctreeChunk* chunk;

		Ogre::Vector3 v = cell.toVector3() / (1<<lodLevel);
		v.x = Ogre::Math::Floor(v.x);
		v.y = Ogre::Math::Floor(v.y);
		v.z = Ogre::Math::Floor(v.z);

		Int3 index;
		index.fromVector3(v);

		ChunkMap::iterator it = mChunks.find(index);
		if(it != mChunks.end())
		{
			chunk = it->second;
		}
		else
		{
			return NULL;
		}

		Int3 subpart = cell - (index * (1<<lodLevel));

		for(int i=0; i<lodLevel; i++)
		{
			// subpart / ... 8 4 2 1
			v = subpart.toVector3() / (1 << (lodLevel-i-1));
			v.x = Ogre::Math::Floor(v.x);
			v.y = Ogre::Math::Floor(v.y);
			v.z = Ogre::Math::Floor(v.z);

			index.fromVector3(v);

			subpart -= index * (1 << (lodLevel-i-1));

			chunk = chunk->getChild(index.x + index.y*2 + index.z*4);
			
			if(!chunk)
			{
				return NULL;
			}
		}

		return chunk;
	}

	void LodManager::associateSiblings(OctreeChunk* chunk, const Ogre::uint8 lodLevel)
	{
		if(chunk->getLodLevel() == 0)
		{
			mChunks.insert(std::pair<Int3, OctreeChunk*>(chunk->getIndex(), chunk));
		}

		for(int i=0; i<6; i++)
		{
			Int3 siblingIndex = chunk->getIndex() + OctreeChunk::msFace[i];

			OctreeChunk* sibling = getChunk(siblingIndex, lodLevel);

			if(sibling)
			{
				int reverse = (i%2 == 1) ? i -1 : i + 1;
				chunk->mSibling[reverse] = sibling;

				sibling->mSibling[i] = chunk;
			}
		}		
	}

	void LodManager::reload()
	{
		OctreeChunkCreatorM2::reload();	// reload density function

		mLastScopePosition = Ogre::Math::POS_INFINITY;
		for(ChunkMap::iterator it = mChunks.begin(); it != mChunks.end(); ++it)
		{
			destroy(it->second, true); // delete the creation data as well
		}
		mChunks.clear();
	}
    
	unsigned long LodManager::getTimeSinceUpdateStarted()
	{
		return Ogre::Root::getSingleton().getTimer()->getMilliseconds() - mTimePointStart;
	}

    void LodManager::update()
    {
		VT::Root* root = VT::Root::getSingletonPtr();
		mTimePointStart = Ogre::Root::getSingleton().getTimer()->getMilliseconds();

        Ogre::uint32 count = ProceduralRenderable::GenNameCount();
        
		//if(mLodCamera[0]->getPosition().squaredDistance(mLastScopePosition) > mViewShiftSquared)
		{
			outScopeChunks();
			inScopeChunks();

			if(getTimeSinceUpdateStarted() > 100)
				return;
		}


		//Ogre::LogManager::getSingletonPtr()->logMessage("deltaTime " + Ogre::StringConverter::toString(getTimeSinceUpdateStarted()));


		/*
        if(camera->getDerivedPosition().squaredDistance(mLastScopePosition) > mViewShiftSquared)
        {
            scopeChunks();

            if(count != ProceduralRenderable::GenNameCount())
            {
                updateFromCamera(camera);
                return;
            }
        }
		

		for(ChunkMap::iterator it = mChunks.begin(); it != mChunks.end(); ++it)
        {
			adjustLod(it->second, camera, 0);
        }
		*/

        for(int lodLimit=1; lodLimit<root->getLodCount(); lodLimit++)
        {   
            for(ChunkMap::iterator it = mChunks.begin(); it != mChunks.end(); ++it)
            {
                createRegular(it->second, lodLimit);
            }
            
            for(ChunkMap::iterator it = mChunks.begin(); it != mChunks.end(); ++it)
            {
                createTransitions(it->second, lodLimit);
            }
            
			/*
            if(count != ProceduralRenderable::GenNameCount())
            {
                update();
                return;
            }
			*/
            
			/*
            for(std::vector<Ogre::Sphere>::iterator it = mLodSphere.begin(); it != mLodSphere.end(); ++it)
            {
                it->setCenter(camera->getDerivedPosition());
            }
			*/

            for(ChunkMap::iterator it = mChunks.begin(); it != mChunks.end(); ++it)
            {
                adjustLod(it->second, lodLimit);
            }

			for(ChunkMap::iterator it = mChunks.begin(); it != mChunks.end(); ++it)
            {
				it->second->adjustTransitions(lodLimit);
            }
        }
    }

	void LodManager::outScopeChunks()
	{
		Ogre::Vector3 pos = mLodCamera[0]->getPosition();
		mOutScopeSphere.setCenter(pos);

		ChunkMap::iterator it = mChunks.begin();
        while (it != mChunks.end())
        {
            if(!it->second->getBoundingBox().intersects(mOutScopeSphere))
            {
				destroy(it->second);
				mChunks.erase(it++);
            }
            else
            {
                it++;
            }
        }
	}

	void LodManager::inScopeChunks()
	{
        VT::Root* root = VT::Root::getSingletonPtr();
        
        Ogre::Vector3 chunkSize = root->getLodChunkSize(0);
        //Ogre::Vector3 pos = mViewNode->_getDerivedPosition();
		Ogre::Vector3 pos = mLodCamera[0]->getPosition();
        
        mInScopeSphere.setCenter(pos);
        //Ogre::Sphere lastInScopeSphere(mLastScopePosition, mInScopeSphere.getRadius());
        //mLastScopePosition = pos;
        
		Vector3 radius(mInScopeSphere.getRadius());

		Vector3 temp = (pos - radius)/chunkSize;
        Int3 volumeMin(Math::IFloor(temp.x), Math::IFloor(temp.y), Math::IFloor(temp.z));
		temp = (pos + radius)/chunkSize;
        Int3 volumeMax(Math::IFloor(temp.x + 1), Math::IFloor(temp.y + 1), Math::IFloor(temp.z + 1));
        

		Ogre::AxisAlignedBox aab;
        Int3 index;
		for(index.x=volumeMin.x; index.x<=volumeMax.x; index.x++)
		{
			for(index.y=volumeMin.y; index.y<=volumeMax.y; index.y++)
			{
				for(index.z=volumeMin.z; index.z<=volumeMax.z; index.z++)
				{  
					aab.setMinimum(index.toVector3() * chunkSize);
					aab.setMaximum(index.toVector3() * chunkSize + chunkSize);

					bool inScope = aab.intersects(mInScopeSphere);

					// does the chunk already exist
					ChunkMap::iterator it = mChunks.find(index);
					if(it != mChunks.end())
					{
						it->second->setVisible(inScope);
					}

					else if(inScope)
					{
						// note associateSiblings() called from creator is where he chunk needs to be added into the map

						OctreeChunk* chunk = createRecursive(index);
						//OctreeChunk* chunk = create(index, NULL, false);
						if(chunk)
						{
							chunk->setVisible(true);
							if(getTimeSinceUpdateStarted() > 100)
								return;
						}
					}
				}
			}
		}
    }
    
    void LodManager::createRegular(OctreeChunk* chunk, const Ogre::uint8 lodLimit)
    {
        if(chunk->getLodLevel() < lodLimit)
        {

			//mLodSphere[chunk->getLodLevel()+1].setCenter(camera->getDerivedPosition());
			//if(mLodSphere[chunk->getLodLevel()+1].intersects(chunk->getBoundingBox()))
			if(isVisible(chunk, chunk->getLodLevel() + 1))
			{
                for(int i=0; i<8; i++)
                {
                    OctreeChunk* child = chunk->createOrRetrieveChild(i);
                    if(child)
                    {
	                      createRegular(child, lodLimit);
                    }
                }
            }
        }
    }

	void LodManager::createTransitions(OctreeChunk* chunk, const Ogre::uint8 lodLimit)
	{
 		//mLodSphere[chunk->getLodLevel()+1].setCenter(camera->getDerivedPosition());
		//if(mLodSphere[chunk->getLodLevel()+1].intersects(chunk->getBoundingBox()))
		if(isVisible(chunk, chunk->getLodLevel()+1))
		{
			// possibility of child having lod transitions?
			if(chunk->getLodLevel()+1 < lodLimit)
			{
				for(int i=0; i<8; i++)
				{
					OctreeChunk* child = chunk->getChild(i);
					if(child)
					{
						createTransitions(child, lodLimit);
					}
				}
			}
		}
		else
		{
			for(int i=0; i<6; i++)
			{
				if(!chunk->getTransition(i) && chunk->mSibling[i])
				{
					// will the sibling be shown at higher lod than the chunk?
					//if(mLodSphere[chunk->getLodLevel()+1].intersects(chunk->mSibling[i]->getBoundingBox()))
					if(isVisible(chunk->mSibling[i], chunk->getLodLevel()+1))
					{
						chunk->createOrRetrieveTransition(i);
					}
				}
			}
		}
	}

    
    void LodManager::adjustLod(OctreeChunk* chunk, const Ogre::uint8 lodLimit)
    {
        if(chunk->getLodLevel() <= lodLimit)
        {
			// If the 8th quadrant has had a creation attempt then the 7 other quadrants will have neccessarilly had creation atempts.
			// Best we can do is show this lod
			if(chunk->mChildNeedsCreation[7])
			{
				 chunk->setVisible(true);
			}


			// is visible at next lod?
			//else if(mLodSphere[chunk->getLodLevel()+1].intersects(chunk->getBoundingBox()))
			else if(isVisible(chunk, chunk->getLodLevel()+1))
			{
				chunk->setVisible(false);
               
				for(int i=0; i<8; i++)
				{
					OctreeChunk* child = chunk->getChild(i);
                   
					if(child)
					{
						// child is at highest lod
						if(child->getLodLevel() == VT::Root::getSingleton().getLodCount() - 1)
						{
							child->setVisible(true);
						}
						else
						{
							adjustLod(child, lodLimit);
						}
					}
				}
            }
			
            else
            {
                chunk->setVisible(true);
				chunk->hideAllChildren();
            }
        }
    }
}
}