#include "GOOFVTOctreeChunkRaySceneQuery.h"
#include "GOOFVTRoot.h"
#include "GOOFSlicing.h"

using namespace Ogre;

namespace GOOF
{
namespace VT
{
    OctreeChunkRaySceneQuery::OctreeChunkRaySceneQuery()
    {
        mRaycastMat = MaterialManager::getSingleton().load("VT/Raycast", ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME).staticCast<Material>();

		VT::Root* root = VT::Root::getSingletonPtr();
        mRaycastMat->getTechnique(0)->getPass(0)->getGeometryProgramParameters()->addSharedParameters( root->getGpuSharedParameter() );

        // for now a single vertex input and output
        Ogre::ManualObject*  seedObject = getSceneManager()->createManualObject("VT/RaycastSeedObject");
		seedObject->estimateVertexCount(1);
		seedObject->begin("", RenderOperation::OT_POINT_LIST);
        seedObject->position(Vector3::ZERO);
        //seedObject->textureCoord(Vector3::ZERO);
		seedObject->end();

        Ogre::RenderToVertexBufferSharedPtr r2vblistTri = HardwareBufferManager::getSingleton().createRenderToVertexBuffer();
        
		r2vblistTri->setOperationType(RenderOperation::OT_POINT_LIST);
		r2vblistTri->setMaxVertexCount(1);
		r2vblistTri->setResetsEveryUpdate(false);
        
		Ogre::VertexDeclaration* vertexDecl = r2vblistTri->getVertexDeclaration();
		size_t offset = 0;
		offset += vertexDecl->addElement(0, offset, VET_FLOAT1, VES_TEXTURE_COORDINATES, 0).getSize();
        //offset += vertexDecl->addElement(0, offset, VET_FLOAT3, VES_TEXTURE_COORDINATES, 1).getSize();

        r2vblistTri->setSourceRenderable(seedObject->getSection(0));
        r2vblistTri->setRenderToBufferMaterialName(mRaycastMat->getName());

        mRaycastObject = static_cast<ProceduralRenderable*> (getSceneManager()->createMovableObject("OctreeChunkRaySceneQuery", ProceduralRenderableFactory::FACTORY_TYPE_NAME));

        mRaycastObject->setRenderToVertexBuffer(r2vblistTri);
    }

    OctreeChunk* OctreeChunkRaySceneQuery::intersects (const Ogre::Ray& ray, Ogre::Real& dist)
    {
        getSceneQuery()->setRay(ray);
        getSceneQuery()->setSortByDistance(true);
        //query->setTypeMask();
        execute();

        for(std::vector<OctreeChunk*>::iterator it = getResults().begin(); it != getResults().end(); ++it)
        {
            OctreeChunk* chunk = *it;
            assert(chunk->rvs);

            Ogre::Real d1, d2; // near and far intersection distances

			// we end up doing the same large rvs multiple times for each child the ray passes through, so to stop that
			// want the root chunks only! 


            //if(chunk->isVisible() && Ogre::Math::intersects(ray, chunk->getBoundingBox(), &d1, &d2))

			// root chunk
			if(chunk->getParent() == NULL && Ogre::Math::intersects(ray, chunk->getBoundingBox(), &d1, &d2))
            {
				Ogre::Pass* pass = mRaycastMat->getTechnique(0)->getPass(0);
				pass->getTextureUnitState(0)->setTexture(chunk->rvs->getRenderVolume());

                VT::Root* root = VT::Root::getSingletonPtr();
                
                Ogre::Vector3 chunkSize = root->getLodChunkSize(0);
                
                OctreeChunk* rootChunk = chunk;
                while(rootChunk->getParent())
                {
                    rootChunk = rootChunk->getParent();
                }
                
				Ogre::Vector3 origin = rootChunk->getIndex().toVector3() * chunkSize;


				int voxelDim = root->getVoxelDim();
				int voxelDimMinusOne = voxelDim-1;
				//int margin = root->getMargin();
				//int invVoxelDimPlusMargin = 1.0f/(voxelDim + 2*margin -1);
				//int voxelDimPlusMargin = root->getVoxelDimPlusMargin();

				// cell [0.0 - voxelDimMinusOne]
				Ogre::Vector3 rayStartVoxelCoord = voxelDimMinusOne * (ray.getPoint(d1) - origin)/chunkSize;
				Ogre::Vector3 rayEndVoxelCoord   = voxelDimMinusOne * (ray.getPoint(d2) - origin)/chunkSize;


				const Ogre::GpuProgramParametersSharedPtr& params = pass->getGeometryProgramParameters();
				params->setNamedConstant("ray_start_voxel_coord", rayStartVoxelCoord);
				params->setNamedConstant("ray_end_voxel_coord", rayEndVoxelCoord);
				pass->getTextureUnitState(0)->setTexture(chunk->rvs->getRenderVolume());


				float lodScale = (float) 1.0 / (1 << (root->getLodCount()-1));
				float temp[2] = { 0, 0 };
				float invVoxelDimPlusMarginsMinusOne = 1.0 / float(root->getVoxelDim() + 2* root->getMargin());
				temp[0] = lodScale * invVoxelDimPlusMarginsMinusOne;
				root->getGpuSharedParameter()->setNamedConstant("sampler_density_tex_step", temp, 2);


				/*
				Ogre::LogManager::getSingletonPtr()->logMessage("rayStartVoxelCoord " + Ogre::StringConverter::toString(rayStartVoxelCoord));
				Ogre::LogManager::getSingletonPtr()->logMessage("rayEndVoxelCoord " + Ogre::StringConverter::toString(rayEndVoxelCoord));
				Ogre::LogManager::getSingletonPtr()->logMessage("d1 " + Ogre::StringConverter::toString(d1));
				Ogre::LogManager::getSingletonPtr()->logMessage("d2 " + Ogre::StringConverter::toString(d2));
				Ogre::LogManager::getSingletonPtr()->logMessage("sampler_density_tex_step " + Ogre::StringConverter::toString(temp[0]));
				*/

                Ogre::RenderToVertexBufferSharedPtr r2vb = mRaycastObject->getRenderToVertexBuffer();
                r2vb->reset();
                // note : update actually adds a stream out query so can use vertexCount
                r2vb->update(getSceneManager());

				RenderOperation op;
				r2vb->getRenderOperation(op);
	
				if(op.vertexData->vertexCount != 0)
				{
					HardwareVertexBufferSharedPtr vBuf = op.vertexData->vertexBufferBinding->getBuffer(0);
					Real* pReal = static_cast<Real*>(vBuf->lock(HardwareBuffer::HBL_READ_ONLY));
					
					// The proportion of the ray that gets us from d1 to the hit point, along d1 -> d2
					float prop = *pReal++;	
					vBuf->unlock();

					dist = ((d2-d1) * prop) + d1;
					return chunk;
				}
			}
		}

		return NULL;
    }

}
}

