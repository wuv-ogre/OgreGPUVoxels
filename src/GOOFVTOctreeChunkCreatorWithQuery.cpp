#include "GOOFVTOctreeChunkCreatorWithQuery.h"
#include "GOOFVTOctreeChunk.h"
#include "GOOFProceduralRenderable.h"
#include "GOOFDataFileResourceManager.h"
#include "GOOFDataFileResource.h"
#include "GOOFDataElement.h"
#include "GOOFInt3.h"
#include "GOOFVTRoot.h"

//#include "GOOFVTRegional.h"
//#include "GOOFVTMaterialBuilder.h"
//#include "GOOFPrintOgre.h"


using namespace Ogre;

namespace GOOF
{
namespace VT
{
	OctreeChunkQuery OctreeChunkQueryAsVisit::NOT_VISITED = OctreeChunkQuery();

	OctreeChunkCreatorWithQuery::~OctreeChunkCreatorWithQuery()
	{
		for(QueryMap::iterator it = mQueryMap.begin(); it != mQueryMap.end(); ++it)
		{
			delete it->second;
		}
		mQueryMap.clear();
	}

	void OctreeChunkCreatorWithQuery::loadConfigFile(const Ogre::String& filename)
	{
		mLoadedQueryMap = true;

		for(QueryMap::iterator it = mQueryMap.begin(); it != mQueryMap.end(); ++it)
		{
			delete it->second;
		}
		mQueryMap.clear();


		DataFileResourcePtr res = GOOF::DataFileResourceManager::getSingleton().load(filename, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME).staticCast<DataFileResource>();
		DataElementPtr terrainEl = res->getRootElement();
		
		for(DataElementPtr chunkEl = terrainEl->getFirstChild("chunk"); !chunkEl->getIsEmpty(); chunkEl = chunkEl->getNextSibling("chunk"))
		{
			Int3 index = chunkEl->getAttributeInt3("index", Int3::ZERO);

			std::vector<bool> creationList;
			chunkEl->getAttributeBoolVector("creation", creationList);
			
			OctreeChunkQuery* query = new OctreeChunkQuery;
			int idx = 0;
			generateOctreeChunkQuery(query, creationList, idx);
			mQueryMap[index] = query;
		}
	}

	void OctreeChunkCreatorWithQuery::writeConfigFile(const Ogre::String& filename, const Int3& min, const Int3& max)
	{
		SelectorDataFile sdf;
		sdf.createRootElement("terrain");
		DataElementPtr terrainEl = sdf.getRootElement();

		Int3 index;
		for(index.x = min.x; index.x <= max.x; index.x++)
		{
			for(index.y = min.y; index.y <= max.y; index.y++)
			{
				for(index.z = min.z; index.z <= max.z; index.z++)
				{
					OctreeChunk* chunk = create(index, 0, true);
                            
					if(chunk)
					{
						std::vector<bool> creationList;
						generateCreationList(chunk, creationList);
						
						DataElementPtr chunkEl = terrainEl->createChild("chunk");
						chunkEl->setAttributeInt3("index", index);
						chunkEl->setAttributeBoolVector("creation", creationList);

						destroy(chunk);
					}
				}
			}
		}
		sdf.save(filename);
		mQueryMap.clear();
	}

	OctreeChunkQuery* OctreeChunkCreatorWithQuery::getOctreeChunkQuery(OctreeChunk* chunk)
	{
		std::vector<OctreeChunk*> parentList;
		OctreeChunk* chain = chunk;
		while (chain->getParent())
		{
			parentList.push_back(chain);
			chain = chain->getParent();
		}

		OctreeChunkQuery* query = mQueryMap.find(chain->getIndex())->second;

		for(std::vector<OctreeChunk*>::reverse_iterator it = parentList.rbegin(); it != parentList.rend(); ++it)
		{
			query = query->quadrant[ (*it)->getQuadrantIndex() ];
		}

		return query;
	}

	OctreeChunk* OctreeChunkCreatorWithQuery::create(const Int3& index, OctreeChunk* parent, bool rvsStore, Ogre::uint8 quadrant, bool streamOutQuery)
	{

		if(!mLoadedQueryMap)
		{

			if(parent == NULL)
			{

				QueryMap::iterator it = mQueryMap.find(index);
				if(it != mQueryMap.end())
				{
					if(it->second != NULL)
					{
						return OctreeChunkCreatorM2::create(index, parent, rvsStore, quadrant, true);
					}
				}
				else
				{
					return OctreeChunkCreatorM2::create(index, parent, rvsStore, quadrant, true);
				}
			}
			else
			{
				OctreeChunkQuery* query = getOctreeChunkQuery(parent);

				if(query->quadrant[quadrant])
				{
					return OctreeChunkCreatorM2::create(index, parent, rvsStore, quadrant, true);
				}
			}
		}

		/*
		// build query map as we go
		if(!mLoadedQueryMap)
		{
			OctreeChunk* chunk = NULL;

			if(parent == NULL)
			{
				QueryMap::iterator it = mQueryMap.find(index);
				if(it != mQueryMap.end())
				{
					if(it->second != NULL)
					{
						chunk = OctreeChunkCreatorM2::create(index, parent, rvsStore, quadrant, false);
					}
				}
				else
				{
					chunk = OctreeChunkCreatorM2::create(index, parent, rvsStore, quadrant, true);
					if(chunk)
						mQueryMap[index] = new OctreeChunkQueryAsVisit();
					else
						mQueryMap[index] = NULL;
				}
			}
			else
			{
				OctreeChunkQuery* query = getOctreeChunkQuery(parent);

				if(query->quadrant[quadrant])
				{
					bool notVisited = (query->quadrant[quadrant] == (&OctreeChunkQueryAsVisit::NOT_VISITED)) ? true : false;

					chunk = OctreeChunkCreatorM2::create(index, parent, rvsStore, quadrant, notVisited);

					if(notVisited)
					{
						if(chunk)
							query->quadrant[quadrant] = new OctreeChunkQueryAsVisit();
						else
							query->quadrant[quadrant] = NULL;
					}
				}
			}

			return chunk;
		}
		*/
		else
		{
			if(parent == NULL)
			{
				QueryMap::iterator it = mQueryMap.find(index);
				if(it != mQueryMap.end())
				{
					return OctreeChunkCreatorM2::create(index, parent, rvsStore, quadrant, false);
				}
			}
			else
			{
				OctreeChunkQuery* query = getOctreeChunkQuery(parent);
				assert(query);

				if(query->quadrant[quadrant])
				{
					return OctreeChunkCreatorM2::create(index, parent, rvsStore, quadrant, false);
				}
			}
		}

		if(parent)
			parent->mChildNeedsCreation[quadrant] = false;

		return NULL;
	}

	OctreeChunkQuery* OctreeChunkCreatorWithQuery::buildOctreeChunkQuery(OctreeChunkQuery* query, Ogre::RenderToVertexBufferSharedPtr& r2vbListTri, const Int3& index, OctreeChunk* parent, Ogre::uint8 quadrant, const Ogre::Vector3& chunkSize)
	{
		RenderOperation op;
		r2vbListTri->getRenderOperation(op);

		if(op.vertexData->vertexCount != 0)
		{
			HardwareVertexBufferSharedPtr vBuf = op.vertexData->vertexBufferBinding->getBuffer(0);
            Ogre::Real* pReal = static_cast<Ogre::Real*>(vBuf->lock(HardwareBuffer::HBL_READ_ONLY));

			VT::Root* root = VT::Root::getSingletonPtr();

			Ogre::Real invVoxelDimMinusOne = root->getInvVoxelDimMinusOne();
			Ogre::uint voxelDimMinusOne = root->getVoxelDim() - 1;
			Ogre::uint voxelDimMinusTwo = voxelDimMinusOne - 1;
			Ogre::uint halfVoxelDimMinusOne = voxelDimMinusOne/2;

			Ogre::uint8 lodLevel = parent != NULL ? parent->getLodLevel()+1 : 0;
			bool isHighestLod = lodLevel == (root->getLodCount() - 1) ? true : false;
			Ogre::Vector3 wsChunkPos = index.toVector3() * chunkSize;

			if(!query)
				query = new OctreeChunkQuery();

			// special case only one lod
			if(!parent && root->getLodCount() == 1)
			{
				std::pair<Int3, Int3> bounds;
				bounds.first  = Int3(voxelDimMinusOne + 1);
				bounds.second = Int3(-1);

				for(int vertIdx=0; vertIdx<op.vertexData->vertexCount; vertIdx++)
				{
					Ogre::uint val = *pReal++;
					Int3 pos((val >> 4) & 0xF, (val >> 8) & 0xF, (val >> 12) & 0xF);

					for(int i=0; i<3; i++)
					{
						bounds.first[i]  = std::min(pos[i], bounds.first[i]);
						bounds.second[i] = std::max(pos[i], bounds.second[i]);
					}
				}

				bounds.second += 1;
				query->bounds.setMinimum(wsChunkPos + bounds.first.toVector3()  * invVoxelDimMinusOne * chunkSize);
				query->bounds.setMaximum(wsChunkPos + bounds.second.toVector3() * invVoxelDimMinusOne * chunkSize);

				return query;
			}


			if(!isHighestLod)
			{

				std::pair<Int3, Int3> qudrantBounds[8];
				for(int i=0; i<8; i++)
				{
					qudrantBounds[i].first  = Int3(voxelDimMinusOne + 1); // min
					qudrantBounds[i].second = Int3(-1);					  // max
				}

				for(int vertIdx=0; vertIdx<op.vertexData->vertexCount; vertIdx++)
				{
					Ogre::uint val = *pReal++;
					Int3 pos((val >> 4) & 0xF, (val >> 8) & 0xF, (val >> 12) & 0xF);

					// qudrant index the voxel is in
					int q = (pos.x/halfVoxelDimMinusOne) + (pos.y/halfVoxelDimMinusOne)*2 + (pos.z/halfVoxelDimMinusOne)*4;

					for(int i=0; i<3; i++)
					{
						qudrantBounds[q].first[i]  = std::min(pos[i], qudrantBounds[q].first[i]);
						qudrantBounds[q].second[i] = std::max(pos[i], qudrantBounds[q].second[i]);
					}
				}

				// actual bounds of the chunk as aggreate quadrants 
				std::pair<Int3, Int3> bounds;
				bounds.first  = Int3(voxelDimMinusOne + 1);
				bounds.second = Int3(-1);

				for(int i=0; i<8; i++)
				{
					for(int j=0; j<3; j++)
					{
						bounds.first[j]  = std::min(qudrantBounds[i].first[j],  bounds.first[j]);
						bounds.second[j] = std::max(qudrantBounds[i].second[j], bounds.second[j]);
					}

					if(qudrantBounds[i].second != Int3(-1))
					{
						qudrantBounds[i].second += 1;
						query->quadrant[i] = new OctreeChunkQuery(); 
						query->quadrant[i]->bounds.setMinimum(wsChunkPos + qudrantBounds[i].first.toVector3()  * invVoxelDimMinusOne * chunkSize);
						query->quadrant[i]->bounds.setMaximum(wsChunkPos + qudrantBounds[i].second.toVector3() * invVoxelDimMinusOne * chunkSize);
					}
				}

				// which transitions exist
				for(int face=0; face<3; face++)
				{
					if(bounds.second[face] == voxelDimMinusTwo)
					{
						query->transition[face*2] = OctreeChunkQuery::QS_CREATE;
					}
				}
				for(int face=0; face<3; face++)
				{
					if(bounds.first[face] == 0)
					{
						query->transition[face*2 + 1] = OctreeChunkQuery::QS_CREATE;
					}
				}

				// extents for zero lod chunk
				if(!parent)
				{
					bounds.second += 1;
					query->bounds.setMinimum(wsChunkPos + bounds.first.toVector3()  * invVoxelDimMinusOne * chunkSize);
					query->bounds.setMaximum(wsChunkPos + bounds.second.toVector3() * invVoxelDimMinusOne * chunkSize);
				}
			}

			vBuf->unlock();
			query->done = true;
		}
		
		return query;
	}

	bool OctreeChunkCreatorWithQuery::queryRegular(Ogre::RenderToVertexBufferSharedPtr& r2vbListTri, const Int3& index, OctreeChunk* parent, Ogre::uint8 quadrant, const Ogre::Vector3& chunkSize, Ogre::AxisAlignedBox& aab)
	{
		if(parent == NULL)
		{
			QueryMap::iterator it = mQueryMap.find(index);
			if(it != mQueryMap.end())
			{
				if(it->second != NULL)
				{
					aab = it->second->bounds;
					return true;
				}
			}
			else
			{
				OctreeChunkQuery* query = buildOctreeChunkQuery(NULL, r2vbListTri, index, parent, quadrant, chunkSize);
				mQueryMap[index] = query;
				if(query)
				{
					aab = query->bounds;
					return true;
				}
			}
		}
		else
		{
			OctreeChunkQuery* query = getOctreeChunkQuery(parent);

			if(query->quadrant[quadrant])
			{
				aab = query->quadrant[quadrant]->bounds;

				if(!query->quadrant[quadrant]->done)
				{
					buildOctreeChunkQuery(query->quadrant[quadrant], r2vbListTri, index, parent, quadrant, chunkSize);
				}

				return true;
			}
		}

		return false;
	}
            
    ProceduralRenderable* OctreeChunkCreatorWithQuery::createTransition(OctreeChunk* chunk, Ogre::uint8 face, Ogre::uint8 side, bool changeShaderParams, bool streamOutQuery)
	{
		int idx = face*2 + side;

		OctreeChunkQuery* query = getOctreeChunkQuery(chunk);


		if(query->transition[idx] != OctreeChunkQuery::QS_EMPTY)
		{
			return OctreeChunkCreatorM2::createTransition(chunk, face, side, changeShaderParams, false);
		}

		chunk->mTransitionNeedsCreation[idx] = false;
		return NULL;
	}              
			
	void OctreeChunkCreatorWithQuery::generateCreationList(OctreeChunk* chunk, std::vector<bool>& list)
	{
		if(chunk->getLodLevel() < VT::Root::getSingleton().getLodCount()-1)
		{
			for(int i=0; i<6; i++)
			{
				chunk->getTransition(i) ? list.push_back(true) : list.push_back(false);
			}

			for(int i=0; i<8; i++)
			{
				chunk->getChild(i) ? list.push_back(true) : list.push_back(false);

				if(chunk->getChild(i))
				{
					generateCreationList(chunk->getChild(i), list);
				}
			}
		}
	}

	void OctreeChunkCreatorWithQuery::generateOctreeChunkQuery(OctreeChunkQuery* query, std::vector<bool>& list, int& idx, Ogre::uint8 lodLevel)
	{
		if(lodLevel < VT::Root::getSingleton().getLodCount()-1)
		{
			for(int i=0; i<6; i++)
			{
				if(list[idx+i])
					query->transition[i] = OctreeChunkQuery::QS_CREATE;
			}
			idx += 6;

			for(int i=0; i<8; i++)
			{
				idx++;

				if(list[idx-1])
				{
					OctreeChunkQuery* child = new OctreeChunkQuery;
					query->quadrant[i] = child;
					generateOctreeChunkQuery(child, list, idx, lodLevel+1);
				}
			}
		}
	}

	void OctreeChunkCreatorWithQuery::destroy(OctreeChunk* chunk, const bool clearCreationData)
	{
		if(clearCreationData && chunk->getLodLevel() == 0)
		{
			QueryMap::iterator it = mQueryMap.find(chunk->getIndex());
			if(it != mQueryMap.end())
			{
				mQueryMap.erase(it);
			}
		}

		OctreeChunkCreatorM2::destroy(chunk, clearCreationData);
	}
	
} // ends namespace VT
} // ends namespace GOOF
