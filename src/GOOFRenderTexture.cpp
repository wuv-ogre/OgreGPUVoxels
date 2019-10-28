#include "GOOFRenderTexture.h"
#include "GOOFSharedData.h"
#include <OgreHardwarePixelBuffer.h>
#include <OgreRoot.h>
#include <OgreRectangle2D.h>

using namespace Ogre;

namespace GOOF
{
    RenderTexture::RenderTexture(Ogre::Camera* camera, const Ogre::String &name, const Ogre::String &group, uint width, uint height, Ogre::PixelFormat format, int numMipmaps, int usage)
    {
        init(camera, name, group, width, height, format, numMipmaps, usage);
    }
    
    void RenderTexture::init(Ogre::Camera* camera, const Ogre::String &name, const Ogre::String &group, uint width, uint height, Ogre::PixelFormat format, int numMipmaps, int usage)
    {
        mTexture = Ogre::TextureManager::getSingleton().createManual(name, group, Ogre::TEX_TYPE_2D, width, height, numMipmaps, format, usage);
        
        if(mTexture->getNumMipmaps() == 0)
        {
            Ogre::RenderTarget* rt = mTexture->getBuffer()->getRenderTarget();
            rt->setAutoUpdated(false);
            rt->setActive(true);
            
            Ogre::Viewport* vp = rt->addViewport(camera);
            vp->setClearEveryFrame(false);
            vp->setOverlaysEnabled(false);
            
        }
        else
        {
            for(int i=0; i<mTexture->getNumMipmaps(); i++)
            {
                Ogre::RenderTarget* rt = mTexture->getBuffer(0, i)->getRenderTarget();
                rt->setAutoUpdated(false);
                rt->setActive(true);

                Ogre::Viewport* vp = rt->addViewport( camera );
                vp->setClearEveryFrame(false);
                vp->setOverlaysEnabled(false);
            }
        }
        
        clear(Ogre::FBT_COLOUR | Ogre::FBT_DEPTH | Ogre::FBT_STENCIL);
    }
    
    void RenderTexture::render(Ogre::SceneManager* sceneMgr, Ogre::Pass* pass, bool useIdentityView)
    {
        Ogre::RenderOperation ro;        
        GOOF::SharedData::getSingleton().mRect->getRenderOperation(ro);
        
        render(ro, sceneMgr, pass, useIdentityView);
    }
    
    void RenderTexture::render(Ogre::RenderOperation& ro, Ogre::SceneManager* sceneMgr, Ogre::Pass* pass, bool useIdentityView)
    {
        Ogre::Viewport* vp = mTexture->getBuffer()->getRenderTarget()->getViewport(0);
        
        if(useIdentityView)
        {            
            sceneMgr->manualRender(&ro,
                                   pass,
                                   vp,
                                   Ogre::Matrix4::IDENTITY,
                                   Ogre::Matrix4::IDENTITY,
                                   Ogre::Matrix4::IDENTITY,
                                   false);
        }
        else
        {
            Ogre::Camera* camera = vp->getCamera();
            Ogre::Matrix4 world, view, proj;
            
            Ogre::GpuProgramParametersSharedPtr vparams = pass->getVertexProgramParameters();
            if (vparams->_findNamedConstantDefinition("g_far_corner"))
            {
                Ogre::Vector3 farCorner = camera->getViewMatrix(true) * camera->getWorldSpaceCorners()[4];
                vparams->setNamedConstant("g_far_corner", farCorner);
            }
            
            world = Ogre::Matrix4::IDENTITY;
            view = camera->getViewMatrix(true);
            proj = camera->getProjectionMatrixWithRSDepth();
            sceneMgr->manualRender(&ro,
                                   pass,
                                   vp,
                                   world,
                                   view,
                                   proj,
                                   false);
        }
    }

    void RenderTexture::renderMipmaps(Ogre::SceneManager* sceneMgr, Ogre::Pass* pass)
    {
        Ogre::RenderOperation ro;
        GOOF::SharedData::getSingleton().mRect->getRenderOperation(ro);

        for(int i=1; i<mTexture->getNumMipmaps(); i++)
        {
            Ogre::Viewport* vp = mTexture->getBuffer(0, i)->getRenderTarget()->getViewport(0);

            Ogre::GpuProgramParametersSharedPtr fparams = pass->getFragmentProgramParameters();
            fparams->setNamedConstant("mip", (float) i-1);

            sceneMgr->manualRender(&ro,
                                   pass,
                                   vp,
                                   Ogre::Matrix4::IDENTITY,
                                   Ogre::Matrix4::IDENTITY,
                                   Ogre::Matrix4::IDENTITY,
                                   false);
        }
    }
    
    
    void RenderTexture::clear(unsigned int buffers,
                              const Ogre::ColourValue& colour,
                              Ogre::Real depth,
                              unsigned short stencil)
    {
        Ogre::Viewport* vp = mTexture->getBuffer()->getRenderTarget()->getViewport(0);
        Ogre::RenderSystem* rs = Ogre::Root::getSingleton().getRenderSystem();
        rs->_setViewport(vp);
        rs->clearFrameBuffer(buffers, colour);
    }
    
    
    MultiRenderTexture::MultiRenderTexture(Ogre::Camera* camera, const Ogre::String &name, const Ogre::String &group, uint width, uint height, Ogre::uint numTextures, Ogre::PixelFormat format)
    {
        m_mrt = Ogre::Root::getSingleton().getRenderSystem()->createMultiRenderTarget(name);
        for(int i=0; i<numTextures; i++)
        {
            Ogre::TexturePtr tex = TextureManager::getSingleton().createManual(name + Ogre::StringConverter::toString(i), group, TEX_TYPE_2D, width, height, 0, format, TU_RENDERTARGET);
            Ogre::RenderTarget* rt = tex->getBuffer()->getRenderTarget();
            rt->setAutoUpdated(false);
            Ogre::Viewport* vp = rt->addViewport(camera);
            vp->setClearEveryFrame(false);
            vp->setOverlaysEnabled(false);
            
            m_mrt->bindSurface(i, tex->getBuffer()->getRenderTarget());
            
            mTextures.push_back(tex);
        }
		m_mrt->setAutoUpdated(false);
        m_mrt->addViewport(camera);
    }
    
    
    void MultiRenderTexture::render(Ogre::SceneManager* sceneMgr, Ogre::Pass* pass, bool useIdentityView)
    {
        Ogre::RenderOperation ro;
        GOOF::SharedData::getSingleton().mRect->getRenderOperation(ro);
        //ro.srcRenderable = GOOF::SharedData::getSingleton().mRect;
        
        Ogre::Viewport* vp = m_mrt->getViewport(0);

        if(useIdentityView)
        {
            sceneMgr->manualRender(&ro,
                                   pass,
                                   vp,
                                   Ogre::Matrix4::IDENTITY,
                                   Ogre::Matrix4::IDENTITY,
                                   Ogre::Matrix4::IDENTITY,
                                   false);
        }
        
        else
        {
            Ogre::Camera* camera = vp->getCamera();
            Ogre::Matrix4 world, view, proj;
            
            Ogre::GpuProgramParametersSharedPtr vparams = pass->getVertexProgramParameters();
            if (vparams->_findNamedConstantDefinition("g_far_corner"))
            {
                Ogre::Vector3 farCorner = camera->getViewMatrix(true) * camera->getWorldSpaceCorners()[4];
                vparams->setNamedConstant("g_far_corner", farCorner);
            }
            
            world = Ogre::Matrix4::IDENTITY;
            view = camera->getViewMatrix(true);
            proj = camera->getProjectionMatrixWithRSDepth();
            sceneMgr->manualRender(&ro,
                                   pass,
                                   vp,
                                   world,
                                   view,
                                   proj,
                                   false);
        }
    }

    HexGridRenderTexture::HexGridRenderTexture(Ogre::Camera* camera, const Ogre::String &name, const Ogre::String &group, float width, float height, float gridSpace, uint border, Ogre::PixelFormat format) :
        mOffsets(Ogre::Vector4::ZERO),
        mScale(Ogre::Vector3::UNIT_SCALE)
    {
        float h = Ogre::Math::Sqrt(3.0)/2.0 * gridSpace;
        
        mOffsets = Vector4(2*h, h, gridSpace/2, gridSpace);
        
        float cellsXf = width/mOffsets.x  + 0.5;
        float cellsYf = height/mOffsets.z + 1;
        
        int cellsX = (int) cellsXf;
        int cellsY = (int) cellsYf;
        
        mScale.x += (cellsXf - cellsX)/cellsX;
        mScale.z += (cellsYf - cellsY)/cellsY;
        
        cellsX += border;
        cellsY += border*4;

        mTexture = Ogre::TextureManager::getSingleton().createManual(name, group, Ogre::TEX_TYPE_2D, cellsX, cellsY, 0, format, Ogre::TU_RENDERTARGET);
        Ogre::RenderTarget* rt = mTexture->getBuffer()->getRenderTarget();
        rt->setAutoUpdated(false);
        Ogre::Viewport* vp = rt->addViewport(camera);
        vp->setClearEveryFrame(false);
        vp->setOverlaysEnabled(false);
    }
    
    void HexGridRenderTexture::render(Ogre::SceneManager* sceneMgr, Ogre::Pass* pass, bool useIdentityView)
    {
        Ogre::GpuProgramParametersSharedPtr fparams = pass->getFragmentProgramParameters();
        
        if(fparams->_findNamedConstantDefinition("grid_offsets"))
        {
            fparams->setNamedConstant("grid_offsets", mOffsets);
        }
        if(fparams->_findNamedConstantDefinition("grid_scale"))
        {
            fparams->setNamedConstant("grid_scale", mScale);
        }

        Ogre::RenderOperation ro;
        GOOF::SharedData::getSingleton().mRect->getRenderOperation(ro);
        //ro.srcRenderable = GOOF::SharedData::getSingleton().mRect;
        
        Ogre::Viewport* vp = mTexture->getBuffer()->getRenderTarget()->getViewport(0);
        
        if(useIdentityView)
        {
            sceneMgr->manualRender(&ro,
                                   pass,
                                   vp,
                                   Ogre::Matrix4::IDENTITY,
                                   Ogre::Matrix4::IDENTITY,
                                   Ogre::Matrix4::IDENTITY,
                                   false);
        }
        else
        {
            Ogre::Camera* camera = vp->getCamera();
            Ogre::Matrix4 world, view, proj;
            
            Ogre::GpuProgramParametersSharedPtr vparams = pass->getVertexProgramParameters();
            if (vparams->_findNamedConstantDefinition("g_far_corner"))
            {
                Ogre::Vector3 farCorner = camera->getViewMatrix(true) * camera->getWorldSpaceCorners()[4];
                vparams->setNamedConstant("g_far_corner", farCorner);
            }
            
            world = Ogre::Matrix4::IDENTITY;
            view = camera->getViewMatrix(true);
            proj = camera->getProjectionMatrixWithRSDepth();
            sceneMgr->manualRender(&ro,
                                   pass,
                                   vp,
                                   world,
                                   view,
                                   proj,
                                   false);
        }
    }
    

    RenderVolume::RenderVolume(Ogre::Camera* camera, const Ogre::String &name, const Ogre::String &group, uint width, uint height, uint depth, Ogre::PixelFormat format)
    {
		mTexture = TextureManager::getSingleton().createManual(name, group, TEX_TYPE_3D, width, height, depth, 0, format, TU_RENDERTARGET);
        
		for(int i=0; i<mTexture->getDepth(); i++)
		{
			Ogre::RenderTarget* rt = mTexture->getBuffer()->getRenderTarget(i);
			rt->setAutoUpdated(false);
			Ogre::Viewport* vp = rt->addViewport(camera);
			vp->setClearEveryFrame(false);
			vp->setOverlaysEnabled(false);
		}
        
        clear(Ogre::FBT_COLOUR | Ogre::FBT_DEPTH | Ogre::FBT_STENCIL);
    }
    
    void RenderVolume::clear(unsigned int buffers,
                             const Ogre::ColourValue& colour,
                             Ogre::Real depth,
                             unsigned short stencil)
    {
        Ogre::RenderSystem* rs = Ogre::Root::getSingleton().getRenderSystem();
        
        Ogre::uint numSlices = mTexture->getDepth();
		for(int i=0; i<numSlices; i++)
		{
            Ogre::Viewport* vp = mTexture->getBuffer()->getRenderTarget(i)->getViewport(0);
            rs->_setViewport(vp);
            rs->clearFrameBuffer(buffers, colour, depth, stencil);
        }
    }

    void RenderVolume::render(Ogre::SceneManager* sceneMgr, Ogre::Pass* pass)
	{
        Ogre::GpuProgramParametersSharedPtr vparams = pass->getVertexProgramParameters();
        
        Ogre::Rectangle2D* rect = SharedData::getSingleton().mRect;
        Ogre::RenderOperation ro;
        rect->getRenderOperation(ro);
        //ro.srcRenderable = rect;
        
		Ogre::uint depth = mTexture->getDepth();
		for(int i=0; i<depth; i++)
		{
			vparams->setNamedConstant("slice", (float) i);
            
			// get the render target for the slice
			Ogre::RenderTarget* rt = mTexture->getBuffer()->getRenderTarget(i);
            
			sceneMgr->manualRender(&ro,
                                   pass,
                                   rt->getViewport(0),
                                   Ogre::Matrix4::IDENTITY,
                                   Ogre::Matrix4::IDENTITY,
                                   Ogre::Matrix4::IDENTITY,
                                   false);
		}
	}

    void RenderVolume::renderSection(Ogre::SceneManager* sceneMgr, Ogre::Pass* pass, const Ogre::Vector3& texPos, const Ogre::Vector3 texScale)
    {
        Ogre::GpuProgramParametersSharedPtr vparams = pass->getVertexProgramParameters();
        
        // Ogre::Rectangle2D is using the identity transform (to get viewport aligned)
        Ogre::Rectangle2D* rect = SharedData::getSingleton().mRect;
        Ogre::RenderOperation ro;
        rect->getRenderOperation(ro);
        
        int depth = mTexture->getDepth();
        int start = (texPos.z - texScale.z)*depth;
        int end = (texPos.z + texScale.z)*depth + 1;
        start = std::max(start, 0);
        end = std::min(end, depth);
        
        Ogre::Matrix4 trans;
        trans.makeTransform(Ogre::Vector3(texPos.x*2.0-1.0, -(texPos.y*2.0-1.0), 0.0), Ogre::Vector3(texScale.x, texScale.y, 1.0), Ogre::Quaternion::IDENTITY);
        
        bool hasSliceParam = vparams->_findNamedConstantDefinition("slice") ? true : false;
        
        for(Ogre::uint i = start; i<end; i++)
        {
            if(hasSliceParam)
                vparams->setNamedConstant("slice", (float) i/depth);
            
            Ogre::RenderTarget* rt = mTexture->getBuffer()->getRenderTarget(i);
            
            sceneMgr->manualRender(&ro,
                                   pass,
                                   rt->getViewport(0),
                                   trans,                   // << set trans here, world would be identity in the camera
                                   Ogre::Matrix4::IDENTITY,
                                   Ogre::Matrix4::IDENTITY,
                                   false);
        }
    }
    
    void RenderVolume::render(const Ogre::String& compositorName, Ogre::GpuProgramParametersSharedPtr vparams)
	{
		Ogre::Camera* camera = mTexture->getBuffer()->getRenderTarget(0)->getViewport(0)->getCamera();
        
		PolygonMode mode = camera->getPolygonMode();
		camera->setPolygonMode(PM_SOLID);
        
		Ogre::uint depth = mTexture->getDepth();
		for(int i=0; i<depth; i++)
		{
			vparams->setNamedConstant("slice", (float) i);
			
			// render target for the slice
			Ogre::RenderTarget* rt = mTexture->getBuffer()->getRenderTarget(i);
			Ogre::Viewport* vp = rt->getViewport(0);
            
			Ogre::CompositorManager::getSingleton().addCompositor(vp, compositorName);
			Ogre::CompositorManager::getSingleton().setCompositorEnabled(vp, compositorName, true);
            
			rt->update();
            
			Ogre::CompositorManager::getSingleton().removeCompositor(vp, compositorName);
		}
        
		camera->setPolygonMode(mode);
	}
    
    
    
    
    /*
    RenderTexture3d::RenderTexture3d(const Ogre::String &name, const Ogre::String &group, uint width, uint height, uint depth, Ogre::PixelFormat format)
    {
        mTexture = TextureManager::getSingleton().createManual(uniqueName,
                                                               "General",
                                                               TEX_TYPE_3D,
                                                               dim, dim, dim,
                                                               0,
                                                               pixelFormat,
                                                               TU_RENDERTARGET);
        
        Ogre::MultiRenderTarget* mrt = Ogre::Root::getSingleton().getRenderSystem()->createMultiRenderTarget(uniqueName);
        
        for(int i=0; i<mTexture->getDepth(); i++)
        {
            mrt->bindSurface(i, mTexture->getBuffer()->getRenderTarget(i));
        }
        
        Ogre::Viewport* vp = mrt->addViewport(camera);
        vp->setClearEveryFrame(false);
        vp->setOverlaysEnabled(false);
        
        if(!Ogre::MeshManager::getSingleton().resourceExists("TestQuad"))\
        {
            Quad::Quad( "TestQuad" );
        }
        
        Ogre::MeshPtr quad = Ogre::MeshManager::getSingleton().getByName("TestQuad");
        
        InstanceManager* instMgr = sceneMgr->createInstanceManager(uniqueName,
                                                                   quad->getName(),
                                                                   ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME,
                                                                   Ogre::InstanceManager::ShaderBased,
                                                                   mTexture->getDepth(), // as many instances as slices
                                                                   IM_USEALL,
                                                                   0);
        
        for(int i=0; i<mTexture->getDepth(); i++)
        {
            instMgr->createInstancedEntity(mat->getName());
        }
        
        Ogre::InstanceManager::InstanceBatchIterator it = instMgr->getInstanceBatchIterator(mat->getName());
        mBatch = it.getNext();
        
        Ogre::RenderOperation ro;
        mBatch->getRenderOperation(ro);
        
        sceneMgr->manualRender(&ro,
                               mat->getTechnique(0)->getPass(0),
                               vp,
                               Ogre::Matrix4::IDENTITY,
                               Ogre::Matrix4::IDENTITY,
                               Ogre::Matrix4::IDENTITY,
                               true);
    }
    */
}
