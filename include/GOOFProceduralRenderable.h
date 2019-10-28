#ifndef __GOOFProceduralRenderable_H__
#define __GOOFProceduralRenderable_H__

//#include <OgreManualObject.h>
//#include <OgreSimpleRenderable.h>
//#include <OgreRenderToVertexBuffer.h>

#include <Ogre.h>

namespace GOOF
{
	class ProceduralRenderable : public Ogre::SimpleRenderable
	{
	public:
		ProceduralRenderable() {}
		virtual ~ProceduralRenderable() {}

		void setRenderToVertexBuffer(Ogre::RenderToVertexBufferSharedPtr r2vbObject)
			{ mR2vbObject = r2vbObject; }
		const Ogre::RenderToVertexBufferSharedPtr& getRenderToVertexBuffer()
			{ return mR2vbObject; }

		/** The engine will call this method when this object is to be rendered. The
            object must then create one or more Renderable subclass instances which it
            places on the passed in Queue for rendering.
        */
		void _updateRenderQueue(Ogre::RenderQueue* queue);

		/** @copydoc SimpleRenderable::getMovableType. */
		const Ogre::String& getMovableType(void) const;

		/** Gets the render operation required to send this object to the frame buffer.
		*/
		void getRenderOperation(Ogre::RenderOperation& op);

        virtual Ogre::Real getSquaredViewDepth(const Ogre::Camera* cam) const
        {
            return mParentNode->getSquaredViewDepth(cam);
        }

		virtual Ogre::Real getBoundingRadius(void) const
		{
            Ogre::Vector3 half = mBox.getHalfSize();
            return half.length();
		}

        /** Just like the equivalent ManualObject function */
        Ogre::MeshPtr convertToMesh(const Ogre::String& meshName,
                                    const Ogre::String& groupName = Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);

        static Ogre::uint GenNameCount() { return msGenNameCount; }
        
        /** The Ogre::SimpleRenderable render material */
        void setMaterial( const Ogre::MaterialPtr& material )
        {
            mMaterial = material;
            
            // Ensure new material loaded (will not load again if already loaded)
            mMaterial->load();
        }

        void setMaterialName( const Ogre::String& matName )
        {
            Ogre::SimpleRenderable::setMaterial(matName);
        }

        
        /**
        void setManualObject(Ogre::ManualObject* manualObject)
        {
            mManualObject = manualObject;
            mParentSceneManager = manualObject->_getManager();
            if (!mR2vbObject.isNull())
            {
                mR2vbObject->setSourceRenderable(manualObject->getSection(0));
            }
        }
        */
        
	protected:
		Ogre::RenderToVertexBufferSharedPtr mR2vbObject;
        //Ogre::ManualObject* mManualObject;
        
	};


	class ProceduralRenderableFactory : public Ogre::MovableObjectFactory
	{
	protected:
		Ogre::MovableObject* createInstanceImpl(const Ogre::String& name, const Ogre::NameValuePairList* params);

	public:
		ProceduralRenderableFactory() {}
		~ProceduralRenderableFactory() {}

		static Ogre::String FACTORY_TYPE_NAME;

		const Ogre::String& getType(void) const;
		void destroyInstance(Ogre::MovableObject* obj);
	};
}
#endif
