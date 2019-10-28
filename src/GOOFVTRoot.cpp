#include "GOOFVTRoot.h"
//#include "GOOFDataFileResourceManager.h"
//#include "GOOFDataFileResource.h"
//#include "GOOFDataElement.h"
#include "GOOFTextureUtils.h"
#include "GOOFSlicing.h"
#include "Transvoxel.h"

using namespace Ogre;

template<> GOOF::VT::Root* Ogre::Singleton<GOOF::VT::Root>::msSingleton = 0;

namespace GOOF
{
    namespace VT
    {
        Root::Root(int voxelDim, int margin, int lodCount, Ogre::Vector3 lodZeroChunkSize) :
        mVoxelDim(voxelDim),
        mMargin(margin),
        mLodCount(lodCount),
        mLodZeroChunkSize(lodZeroChunkSize)
        {
            int voxelDimMinusOne = voxelDim-1;
            int voxelDimMinusTwo = voxelDim-2;
            float invVoxelDimMinusOne = 1.0/(voxelDim - 1);
            int	voxelDimPlusMarginsMinusOne = voxelDimMinusOne + 2 * margin;
            float invVoxelDimPlusMarginsMinusOne = 1.0 / float(voxelDimPlusMarginsMinusOne);
            float invVoxelDimPlusMargin = 1.0 / float(voxelDim + 2 * margin);
            
            mSharedParams = GpuProgramManager::getSingleton().getSharedParameters("VT/SharedParams");
            
            // constant shared params
            float temp[2] = { 0, 0 };
            mSharedParams->setNamedConstant("g_voxel_dim", (float) voxelDim);
            mSharedParams->setNamedConstant("g_margin",  (float) margin);
            mSharedParams->setNamedConstant("g_voxel_dim_minus_one", (float) voxelDimMinusOne);
            mSharedParams->setNamedConstant("g_voxel_dim_minus_two", (float) voxelDimMinusTwo);
            temp[0] = 1.0f/voxelDim;
            mSharedParams->setNamedConstant("g_inv_voxel_dim", temp, 2);
            temp[0] = invVoxelDimMinusOne;
            mSharedParams->setNamedConstant("g_inv_voxel_dim_minus_one", temp, 2);
            temp[0] = voxelDim + 2*margin;
            mSharedParams->setNamedConstant("g_voxel_dim_plus_margin", temp, 2);
            temp[0] = 1.0f/(voxelDim + 2*margin);
            mSharedParams->setNamedConstant("g_inv_voxel_dim_plus_margin", temp, 2);
            temp[0] = 1.0f/(voxelDim + 2*margin -1);
            mSharedParams->setNamedConstant("g_inv_voxel_dim_plus_margin_minus_one", temp, 2);
            mSharedParams->setNamedConstant("g_lod_count", (float) lodCount);
            mSharedParams->setNamedConstant("g_transition_width", (float) 0.5);
            
            Ogre::uint rvs_size = getRenderVolumeSize();
            temp[0] = (1.0/rvs_size);
            mSharedParams->setNamedConstant("g_inv_density_volume_size", temp, 2);
            
            adjustSharedParamters(Int3::ZERO, 0);
            
            mResourcesMat = MaterialManager::getSingleton().create("VT/Resources", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
            Pass* pass = mResourcesMat->getTechnique(0)->getPass(0);
            
            TexturePtr tex;
            TextureUnitState* tunit;
            
            // Regular Triangle Table
            tex = createRegularTriTableTexture();
            tunit = pass->createTextureUnitState();
            tunit->setTextureName(tex->getName(), TEX_TYPE_1D);
            tunit->setTextureAddressingMode(TextureUnitState::TAM_CLAMP);
            tunit->setTextureAnisotropy(1);	// no anisotrophy
            tunit->setTextureFiltering(TFO_NONE);
            mTextureUnitStates.push_back(tunit);
            
            // Transition Triangle Table
            tex = createTransitionTriTableTexture();
            tunit = pass->createTextureUnitState();
            tunit->setTextureName(tex->getName(), TEX_TYPE_1D);
            tunit->setTextureAddressingMode(TextureUnitState::TAM_CLAMP);
            tunit->setTextureAnisotropy(1);	// no anisotrophy
            tunit->setTextureFiltering(TFO_NONE);
            mTextureUnitStates.push_back(tunit);
            
			// Perlin Noise Volumes
			static const Ogre::String NoiseVolumeTexNames[8] =
            {
                "noise_half_16cubed_mips_00.vol",
                "noise_half_16cubed_mips_01.vol",
                "noise_half_16cubed_mips_02.vol",
                "noise_half_16cubed_mips_03.vol",
                "packednoise_half_16cubed_mips_00.vol",
                "packednoise_half_16cubed_mips_01.vol",
                "packednoise_half_16cubed_mips_02.vol",
                "packednoise_half_16cubed_mips_03.vol"
            };
            
            for(int i=0; i<8; i++)
            {
                TexturePtr tex = TextureUtils::LoadRaw(NoiseVolumeTexNames[i],
                                                       Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                                                       16, // width
                                                       16, // height
                                                       16, // depth
                                                       1, // num faces
                                                       4, // num mipmaps
                                                       PF_FLOAT16_RGBA,
                                                       20);	// header
                
                TextureUnitState* tunit = pass->createTextureUnitState();
                tunit->setTextureName(tex->getName(), TEX_TYPE_3D);
                tunit->setTextureAddressingMode(TextureUnitState::TAM_WRAP);
                tunit->setNumMipmaps(0);
                
                if(i<4)
                {
                    tunit->setTextureFiltering(TFO_TRILINEAR);
                }
                else
                {
                    tunit->setTextureFiltering(TFO_NONE);
                }
                
                mTextureUnitStates.push_back(tunit);
            }
            
			// 3D Random Noise Volume
			tex = TextureUtils::LoadRaw("noise_random_32.vol",
                                        Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                                        32, // width
                                        32, // height
                                        32, // depth
                                        1, // num faces
                                        0, // num mipmaps
                                        PF_FLOAT16_RGBA,
                                        0);	// header
            
            tunit = pass->createTextureUnitState();
            tunit->setTextureName(tex->getName(), TEX_TYPE_3D);
            tunit->setTextureAddressingMode(TextureUnitState::TAM_WRAP);
            tunit->setTextureFiltering(TFO_NONE);
            tunit->setNumMipmaps(0);
            mTextureUnitStates.push_back(tunit);
            
            
			// Start indices
			mTextureUnitStatesIndex[0] = 0;		// TriTable
            mTextureUnitStatesIndex[1] = 1;		// TransitionTriTable
            mTextureUnitStatesIndex[2] = 2;		// PerlinNoise
            mTextureUnitStatesIndex[3] = 6;		// PackedPerlinNoise
            mTextureUnitStatesIndex[4] = 10;	// RandomNoise
            mTextureUnitStatesIndex[5] = 11;	// End
            
            
			// Geometry seed object
			mSeedObject = getSceneManager()->createManualObject("VT/SeedObject");
            mSeedObject->estimateVertexCount(voxelDimMinusOne * voxelDimMinusOne * voxelDimMinusOne);
            mSeedObject->begin("", RenderOperation::OT_POINT_LIST);
            
            for(int d=0; d<voxelDimMinusOne; d++)
            {
                for(int h=0; h<voxelDimMinusOne; h++)
                {
                    for(int w=0; w<voxelDimMinusOne; w++)
                    {
                        mSeedObject->position(Vector3(w, h, d));
                        
                        mSeedObject->textureCoord(Vector3((margin + w) * invVoxelDimPlusMarginsMinusOne,
                                                          (margin + h) * invVoxelDimPlusMarginsMinusOne,
                                                          (margin + d) * invVoxelDimPlusMarginsMinusOne));
                        
                        
                    }
                }
            }
            mSeedObject->end();
            
            
			// Transition geometry seed object
			mTransitionSeedObject = getSceneManager()->createManualObject("VT/TransitionSeedObject");
            mTransitionSeedObject->estimateVertexCount(voxelDimMinusOne * voxelDimMinusOne * voxelDimMinusOne);
            mTransitionSeedObject->begin("", RenderOperation::OT_POINT_LIST);
            
            
            for(int h=0; h<voxelDimMinusOne; h++)
            {
                for(int w=0; w<voxelDimMinusOne; w++)
                {
                    mTransitionSeedObject->position(Vector3(w,
                                                            h,
                                                            0));
                    
                    mTransitionSeedObject->textureCoord(Vector3((margin + w) * invVoxelDimPlusMarginsMinusOne,
                                                                (margin + h) * invVoxelDimPlusMarginsMinusOne,
                                                                0));
                }
            }
            
            mTransitionSeedObject->end();
            
            
            
            
            
            /*
             // Regular Triangle Table
             std::vector<ColourValue> colourValues;
             DataFileResourcePtr res = DataFileResourceManager::getSingleton().load("Tables.xml", ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME);
             DataElementPtr rootEl = res->getRootElement();
             
             for(DataElementPtr tableEl = rootEl->getFirstChild("table"); !tableEl->getIsEmpty(); tableEl = tableEl->getNextSibling("table"))
             {
             if(tableEl->getAttributeString("name", "") == "g_triTable_Langley")
             {
             std::vector<int> intVector;
             tableEl->getAttributeIntergerVector("value", intVector);
             
             for (int i=0; i<intVector.size(); i = i+4)
             {
             // floating point value version
             ColourValue cv((float)intVector[i], (float)intVector[i+1], (float)intVector[i+2], 1.0);
             
             for(int j=0; j<3; j++)
             {
             if(cv[j] < 0.0)
             cv[j] = 0.0;
             }
             colourValues.push_back(cv);
             }
             }
             }
             
             int width  = colourValues.size();
             int height = 1;
             tex = TextureManager::getSingleton().createManual("TriTableGBuffer",
             Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
             Ogre::TEX_TYPE_1D,
             width, height, 0, Ogre::PF_FLOAT16_RGB, Ogre::TU_DEFAULT);
             
             TextureUtils::Write(tex, Image::Box(0, 0, width, height), colourValues);
             tunit = pass->createTextureUnitState();
             tunit->setTextureName(tex->getName(), TEX_TYPE_1D);
             tunit->setTextureAddressingMode(TextureUnitState::TAM_CLAMP);
             tunit->setTextureAnisotropy(1);	// no anisotrophy
             tunit->setTextureFiltering(TFO_NONE);
             mTextureUnitStates.push_back(tunit);
             
             // Transition Triangle Table
             colourValues.clear();
             for(DataElementPtr tableEl = rootEl->getFirstChild("table"); !tableEl->getIsEmpty(); tableEl = tableEl->getNextSibling("table"))
             {
             if(tableEl->getAttributeString("name", "") == "g_triTableTransition")
             {
             std::vector<int> intVector;
             tableEl->getAttributeIntergerVector("value", intVector);
             
             for (int i=0; i<intVector.size(); i = i+4)
             {
             // floating point value version
             ColourValue cv((float)intVector[i], (float)intVector[i+1], (float)intVector[i+2], 1.0);
             
             for(int j=0; j<3; j++)
             {
             if(cv[j] < 0.0)
             cv[j] = 0.0;
             }
             colourValues.push_back(cv);
             }
             }
             }
             
             int width  = colourValues.size();
             int height = 1;
             tex = TextureManager::getSingleton().createManual("TransitionTriTableGBuffer",
             Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
             Ogre::TEX_TYPE_1D,
             width, height, 0, Ogre::PF_FLOAT16_RGB, Ogre::TU_DEFAULT);
             
             TextureUtils::Write(tex, Image::Box(0, 0, width, height), colourValues);
             tex = createTransitionTriTableTexture();
             tunit = pass->createTextureUnitState();
             tunit->setTextureName(tex->getName(), TEX_TYPE_1D);
             tunit->setTextureAddressingMode(TextureUnitState::TAM_CLAMP);
             tunit->setTextureAnisotropy(1);	// no anisotrophy
             tunit->setTextureFiltering(TFO_NONE);
             mTextureUnitStates.push_back(tunit);
             */
        }
        
        Root::~Root()
        {
            getSceneManager()->destroyManualObject(mSeedObject);
            getSceneManager()->destroyManualObject(mTransitionSeedObject);
        }
        
        Ogre::TexturePtr Root::createRegularTriTableTexture()
        {
            int EdgeStartInd[12] =
            {
                0, 4, 1, 0, 2, 6, 3, 2, 0, 4, 5, 1,
            };
            
            int EdgeEndInd[12] =
            {
                4, 5, 5, 1, 6, 7, 7, 3, 2, 6, 7, 3,
            };
            
            int width = 256 * 5; // cube case * max num triangles
            int height = 1;
            TexturePtr tex = TextureManager::getSingleton().createManual("RegularTriTableTexture",
                                                                         Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                                                                         Ogre::TEX_TYPE_1D,
                                                                         width, height, 0, Ogre::PF_FLOAT16_RGB, Ogre::TU_DEFAULT);
            
            const PixelBox& pb = tex->getBuffer()->lock(Image::Box(0, 0, width, height), HardwareBuffer::HBL_DISCARD);
            
            // a single byte pointer
            Ogre::uint8* dest = static_cast<Ogre::uint8*>(pb.data);
            size_t pixelStep = PixelUtil::getNumElemBytes(pb.format);
            
            for(int i=0; i<256; i++)
            {
                uint32 tableIndex = i;
                
                uint32 classIndex = regularCellClass[tableIndex];
                const RegularCellData* data = &regularCellData[classIndex];
                
                for (uint16 ind = 0; ind < 15; ind+=3)
                {
                    ColourValue cv(-1,-1,-1,-1);
                    
                    if(ind < data->GetTriangleCount()*3)
                    {
                        for(int j=0; j<3; j++)
                        {
                            uint16 data2 = regularVertexData[tableIndex][ data->vertexIndex[ind+j] ];
                            
                            uint16 corner0 = data2 & 0x0F;
                            uint16 corner1 = (data2 & 0xF0) >> 4;
                            
                            for(int k=0; k<12; k++)
                            {
                                if((EdgeStartInd[k] == corner0 && EdgeEndInd[k] == corner1) ||
                                   (EdgeStartInd[k] == corner1 && EdgeEndInd[k] == corner0)
                                   )
                                {
                                    cv[j] = k;
                                }
                            }
                        }
                    }
                    
                    PixelUtil::packColour(cv, pb.format, dest);
                    dest += pixelStep;
                }
            }
            
            tex->getBuffer()->unlock();
            
            return tex;
        }
        
        Ogre::TexturePtr Root::createTransitionTriTableTexture()
        {
            uint16 EdgeStartInd[20] =
            {
                0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 6, 6, 7, 8, 9, 9, 10, 11
            };
            
            uint16 EdgeEndInd[20] =
            {
                1, 3, 9, 2, 4, 5, 10, 4, 6, 5, 7, 8, 7, 11, 8, 12, 10, 11, 12, 12
            };
            
            
            int width = 512 * 12; // cube case * max num triangles
            int height = 1;
            TexturePtr tex = TextureManager::getSingleton().createManual("TransitionTriTableTexture",
                                                                         Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                                                                         Ogre::TEX_TYPE_1D,
                                                                         width, height, 0, Ogre::PF_FLOAT16_RGB, Ogre::TU_DEFAULT);
            
            const PixelBox& pb = tex->getBuffer()->lock(Image::Box(0, 0, width, height), HardwareBuffer::HBL_DISCARD);
            
            // a single byte pointer
            Ogre::uint8* dest = static_cast<Ogre::uint8*>(pb.data);
            size_t pixelStep = PixelUtil::getNumElemBytes(pb.format);
            
            for(int i=0; i<512; i++)
            {
                uint32 tableIndex = i;
                
                uint32 classIndex = transitionCellClass[tableIndex];
                const TransitionCellData* data = &transitionCellData[classIndex & 0x7F]; // only the last 7 bit count
                
                for (uint16 ind = 0; ind < 36; ind+=3)
                {
                    ColourValue cv(-1,-1,-1,-1);
                    
                    if(ind < data->GetTriangleCount()*3)
                    {
                        for(int j=0; j<3; j++)
                        {
                            uint16 data2 = transitionVertexData[tableIndex][ data->vertexIndex[ind+j] ];
                            
                            uint16 corner0 = data2 & 0x0F;
                            uint16 corner1 = (data2 & 0xF0) >> 4;
                            
                            int edgeInd = -1;
                            for(int k=0; k<20; k++)
                            {
                                if((EdgeStartInd[k] == corner0 && EdgeEndInd[k] == corner1) ||
                                   (EdgeStartInd[k] == corner1 && EdgeEndInd[k] == corner0))
                                {
                                    cv[j] = k;
                                }
                            }
                        }
                    }
                    
                    PixelUtil::packColour(cv, pb.format, dest);
                    dest += pixelStep;
                }
            }
            
            tex->getBuffer()->unlock();
            
            return tex;
        }
        
        TextureUnitState* Root::GetTextureUnitState(const TextureType type, const Ogre::uint index)
        {
            return mTextureUnitStates[ mTextureUnitStatesIndex[(int)(type)] + index ];
        }
        
        TextureUnitState* Root::CreateTextureUnitState(Ogre::Pass* pass, const TextureType type, const Ogre::uint index)
        {
            TextureUnitState* tunit = pass->createTextureUnitState();
            *tunit = *(GetTextureUnitState(type, index));
            return tunit;
        }
        
        void Root::CreateTextureUnitStates(Ogre::Pass* pass, const Ogre::uint types)
        {
            for(int i = 0; i < 5; i++)
            {
                if (types & 1<<i)
                {
                    for(int ind = mTextureUnitStatesIndex[i]; ind < mTextureUnitStatesIndex[i+1]; ind++)
                    {
                        TextureUnitState* tunit = pass->createTextureUnitState();
                        *tunit = *(mTextureUnitStates[ind]);
                    }
                }
            }
        }
        
        void Root::adjustSharedParamters(const Int3& index, Ogre::uint8 lodLevel)
        {
            GOOF::Int3 quadrantOffset((unsigned int)index.x % (1 << lodLevel),
                                      (unsigned int)index.y % (1 << lodLevel),
                                      (unsigned int)index.z % (1 << lodLevel));
            
            Ogre::Vector3 chunkSize = getLodChunkSize(lodLevel);
            
            mSharedParams->setNamedConstant("g_world_space_chunk_pos", index.toVector3() * chunkSize);
            mSharedParams->setNamedConstant("g_world_space_chunk_size", chunkSize);
            mSharedParams->setNamedConstant("g_world_space_voxel_size", getLodVoxelSize(lodLevel));
            mSharedParams->setNamedConstant("g_lod_index", (float) lodLevel);
            
            float lodScale = (float) 1.0 / (1 << lodLevel);
            mSharedParams->setNamedConstant("g_lod_scale", lodScale);
            mSharedParams->setNamedConstant("g_lod_quadrant_offset_uvw", getLodQuadrantOffset_uwv(lodLevel, quadrantOffset));
            
            float temp[2] = { 0, 0 };
            float invVoxelDimPlusMarginsMinusOne = 1.0 / float(getVoxelDim() + 2*getMargin());
            temp[0] = lodScale * invVoxelDimPlusMarginsMinusOne;
            mSharedParams->setNamedConstant("g_sampler_density_tex_step", temp, 2);
        }
        
        
        
    }
}
