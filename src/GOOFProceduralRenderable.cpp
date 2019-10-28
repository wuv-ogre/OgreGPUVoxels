#include "GOOFProceduralRenderable.h"
#include <OgreRenderToVertexBuffer.h>

using namespace Ogre;

namespace GOOF
{
	const String& ProceduralRenderable::getMovableType(void) const
	{
		return ProceduralRenderableFactory::FACTORY_TYPE_NAME;
	}
	//-----------------------------------------------------------------------------
	void ProceduralRenderable::_updateRenderQueue(RenderQueue* queue)
	{
        // If want to update the the RenderToVertexBuffer each frame this was the case with ParticlesGS
        //mR2vbObject->update(mParentSceneManager);

        if (mRenderQueueIDSet)
		{
			queue->addRenderable(this, mRenderQueueID);
		}
		else
		{
			queue->addRenderable(this);
		}
	}
	//-----------------------------------------------------------------------------
	void ProceduralRenderable::getRenderOperation(RenderOperation& op)
	{
		mR2vbObject->getRenderOperation(op);
	}
    //-----------------------------------------------------------------------------
	MeshPtr ProceduralRenderable::convertToMesh(const String& meshName, const String& groupName)
	{
        Ogre::RenderOperation rop;
        getRenderToVertexBuffer()->getRenderOperation(rop);

        if(rop.vertexData->vertexCount == 0)
        {
            MeshPtr ret;
            ret.setNull();
            return ret;
        }

		MeshPtr m = MeshManager::getSingleton().createManual(meshName, groupName);

        SubMesh* sm = m->createSubMesh();
        sm->useSharedVertices = false;
        sm->operationType = rop.operationType;

        sm->setMaterialName(getMaterial()->getName());
        // Copy vertex data; replicate buffers too

        sm->vertexData = rop.vertexData->clone(true);
        // Copy index data; replicate buffers too; delete the default, old one to avoid memory leak        // check if index data is present
        if (rop.indexData)
        {
            // Copy index data; replicate buffers too; delete the default, old one to avoid memory leaks
            OGRE_DELETE sm->indexData;
            sm->indexData = rop.indexData->clone(true);
        }

        // update bounds
		m->_setBounds(getBoundingBox());
		m->_setBoundingSphereRadius(getBoundingRadius());

		m->load();

		return m;
	}


	//-----------------------------------------------------------------------------
	String ProceduralRenderableFactory::FACTORY_TYPE_NAME = "ProceduralRenderable";
	//-----------------------------------------------------------------------------
	const String& ProceduralRenderableFactory::getType(void) const
	{
		return FACTORY_TYPE_NAME;
	}
	//-----------------------------------------------------------------------------
	MovableObject* ProceduralRenderableFactory::createInstanceImpl(
		const String& name, const NameValuePairList* params)
	{
		return OGRE_NEW ProceduralRenderable();
	}
	//-----------------------------------------------------------------------------
	void ProceduralRenderableFactory::destroyInstance( MovableObject* obj)
	{
		OGRE_DELETE obj;
	}

}
