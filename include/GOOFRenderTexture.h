#ifndef __GOOFRenderTexture_H
#define __GOOFRenderTexture_H

#include <Ogre.h>

namespace GOOF
{
    class RenderTexture
    {
    public:
        /**
         * create render texture
        */
        RenderTexture(Ogre::Camera* camera, const Ogre::String &name, const Ogre::String &group, uint width, uint height, Ogre::PixelFormat format, int numMipmaps = 0, int usage=Ogre::TU_RENDERTARGET );

        /**
         * manually setup the texture
         */
        RenderTexture() {}
        
        /**
         * create render texture 
        */
        void init(Ogre::Camera* camera, const Ogre::String &name, const Ogre::String &group, uint width, uint height, Ogre::PixelFormat format, int numMipmaps = 0, int usage=Ogre::TU_RENDERTARGET);
        
        /**
         * @param useIdentityView we can either use an identity view, in which case all shaders will have world view proj = identity
         * or we can use the real camera world view proj, in which case we need a custom vertex shader which will force the rectangle to
         * cover the viewport
         */
        void render(Ogre::SceneManager* sceneMgr, Ogre::Pass* pass, bool useIdentityView = true);
        
        /**
         * @param ro typically want the unit Ogre::Rectangle2D if thats the case just call ^
        */
        void render(Ogre::RenderOperation& ro, Ogre::SceneManager* sceneMgr, Ogre::Pass* pass, bool useIdentityView = true);
        
        /**
         * basic idea is to write a shader which has as input the render texture, we use the mipmap one level higher used to generate lower level
         * with textureLod and fragment param "mip"
        */
        void renderMipmaps(Ogre::SceneManager* sceneMgr, Ogre::Pass* pass);
        
        /**
         * clears the framebuffer colour
         */
        void clear(unsigned int buffers,
                   const Ogre::ColourValue& colour = Ogre::ColourValue::Black,
                   Ogre::Real depth = 1.0f,
                   unsigned short stencil = 0);
        
        Ogre::TexturePtr mTexture;
    };
        
    
    // might need to add clear()
    class MultiRenderTexture
    {
    public:
        MultiRenderTexture(Ogre::Camera* camera, const Ogre::String &name, const Ogre::String &group, uint width, uint height, Ogre::uint numTextures, Ogre::PixelFormat format = Ogre::PF_FLOAT32_RGBA);
        
        Ogre::TexturePtr& getTexture(const Ogre::uint8 ind) { return mTextures[ind]; }
        
        /**
         * @param useIdentityView we can either use an identity view, in which case all shaders will have world view proj = identity
         * or we can use the real camera world view proj, in which case we need a custom vertex shader which will force the rectangle to
         * cover the viewport
         */
        void render(Ogre::SceneManager* sceneMgr, Ogre::Pass* pass, bool useIdentityView = true);
        
    private:
        Ogre::MultiRenderTarget* m_mrt;
        
        /**
         * There is no 1 on 1 relation between Textures and RenderTextures, as there can be multiple RenderTargets
         * rendering to different mipmaps, faces (for cubemaps) or slices (for 3D textures) of the same Texture.
         * & hence must store them
        */
        std::vector<Ogre::TexturePtr> mTextures;

    };
    
    // might need to add clear()
    class HexGridRenderTexture
    {
    public:
        /**
         * @param width The world space width of the grid (not including any border cells)
         * @param height The world space height of the grid (not including any border cells)
         * @param gridSpace The world space distance between points in the hexagonal grid
         * @param border the number of border cells that go outside of width and height bounds,
        */
        HexGridRenderTexture(Ogre::Camera* camera, const Ogre::String &name, const Ogre::String &group, float width, float height, float gridSpace, uint border = 0, Ogre::PixelFormat format = Ogre::PF_FLOAT32_RGBA);
        
        /**
         * @param useIdentityView we can either use an identity view, in which case all shaders will have world view proj = identity
         * or we can use the real camera world view proj, in which case we need a custom vertex shader which will force the rectangle to
         * cover the viewport
         * @note assuming fragment program with params grid_offsets and grid_scale, single technique for useIdentityView = false
         */
        void render(Ogre::SceneManager* sceneMgr, Ogre::Pass* pass, bool useIdentityView = true);
        
        Ogre::TexturePtr mTexture;
        
        // 2*h, h , gridSpace/2 , gridSpace (where h is the height of the equilateral triangle and gridSpace is the triangle side length)
        Ogre::Vector4 mOffsets;
        
        // Scales the grid to the desired size
        Ogre::Vector3 mScale;
    };

    
    /**
     * Renders via a material + quad or alternatively a compositor on each slice of a 3d render target
     * (single pass via material is much quicker)
     */
    class RenderVolume
	{
	public:
        /**
         * @param camera The camera that will be used for viewports in the slices of the volume render target.
		 * @param pixelFormat The pixel format of the volume texture
		 * @param dim The dimensions of the of the volume render target width = height = depth
		 * @param name The name of the volume texture
         */
		RenderVolume(Ogre::Camera* camera, const Ogre::String &name, const Ogre::String &group, uint width, uint height, uint depth, Ogre::PixelFormat format = Ogre::PF_FLOAT32_RGBA);
        
        /**
         * clears the framebuffer colour
         */
        void clear(unsigned int buffers,
                   const Ogre::ColourValue& colour = Ogre::ColourValue::Black,
                   Ogre::Real depth = 1.0f,
                   unsigned short stencil = 0);
        
        /**
         * uses SceneManager::manualRender to render full screen quad on each slice
         */
		virtual void render(Ogre::SceneManager* sceneMgr, Ogre::Pass* pass);
        
        /**
         * renders to a subsection of the render volume
         * @param texPos uv texcoord of the center of the rectangle
         * @param texScale the scale of the rectangle in texCoords
         * example 
         * texPos Ogre::Vector3(0.5, 0.5, 0.5) and texScale = Ogre::Vector3(1.0, 1.0, 1.0) is a full screen quad on every slice
         */
        virtual void renderSection(Ogre::SceneManager* sceneMgr, Ogre::Pass* pass, const Ogre::Vector3& texPos, const Ogre::Vector3 texScale);
        
        /**
         * @param vparams vertex shader parameters that we set the parameter "slice" on
         */
		void render(const Ogre::String& compositorName, Ogre::GpuProgramParametersSharedPtr vparams);
        
		// The volume render target
		Ogre::TexturePtr mTexture;
	};

    
    
    /*
    class RenderVolume
    {
    public:
        RenderTexture3d(Ogre::Camera* camera, const Ogre::String &name, const Ogre::String &group, uint width, uint height, uint depth, Ogre::PixelFormat format = Ogre::PF_FLOAT32_RGBA);
        
        Ogre::TexturePtr mVolume;
        
    private:
        Ogre::MultiRenderTarget* m_mrt;
    };
    */
}
#endif
