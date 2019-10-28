#include "GOOFVT.h"
#include "GOOFProceduralRenderable.h"
#include "GOOFStopwatch.h"
#include "Transvoxel.h"

namespace GOOF
{
    namespace VT
    {
        SharedParams::SharedParams(Ogre::Vector3 lodZeroChunkSize, Ogre::uint8 lodCount, Ogre::uint8 voxelDim, Ogre::uint8 margin)
        {
            setConstantParams(lodZeroChunkSize, lodCount, voxelDim, margin);
            setRegularParams(Int3::ZERO, 0);
            setTransitionParams(0,0);
        }
        
        void SharedParams::setConstantParams(Ogre::Vector3 lodZeroChunkSize, Ogre::uint8 lodCount, Ogre::uint8 voxelDim, Ogre::uint8 margin)
        {
            mVoxelDim = voxelDim;
            mMargin = margin;
            mLodCount = lodCount;
            mLodZeroChunkSize = lodZeroChunkSize;
            mWorldSpaceToTextureSpaceScale = 1.0/(mLodZeroChunkSize + 2 * mMargin * getLodVoxelSize(0) + getLodVoxelSize(0));
            
            mSharedParams = Ogre::GpuProgramManager::getSingleton().getSharedParameters("VT/SharedParams");
            
            Ogre::uint8 voxelDimMinusOne = voxelDim-1;
            Ogre::uint8 voxelDimMinusTwo = voxelDim-2;
            float invVoxelDimMinusOne = 1.0/(voxelDim - 1);
            
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
            
            mSharedParams->setNamedConstant("g_world_space_chunk_size_lod_zero", mLodZeroChunkSize);
            
            
            mSharedParams->setNamedConstant("g_margin_pixels", (float) (margin * (1 << (lodCount-1))) );
        }
        
        void SharedParams::setRegularParams(const Int3& index, Ogre::uint8 lodLevel)
        {
            Ogre::Vector3 chunkSize = getLodChunkSize(lodLevel);
            
            mSharedParams->setNamedConstant("g_world_space_chunk_pos", index.toVector3() * chunkSize);
            mSharedParams->setNamedConstant("g_world_space_chunk_size", chunkSize);
            mSharedParams->setNamedConstant("g_world_space_voxel_size", getLodVoxelSize(lodLevel));
                        
            mSharedParams->setNamedConstant("g_lod_index", (float) lodLevel);
                        
            float lodScale = (float) 1.0 / (1 << lodLevel);
            mSharedParams->setNamedConstant("g_lod_scale", lodScale);
            
            float temp[2] = { 0, 0 };
            float invVoxelDimPlusMargins = 1.0 / float(getVoxelDim() + 2*getMargin() );
            
            temp[0] = lodScale * invVoxelDimPlusMargins;
            mSharedParams->setNamedConstant("g_sampler_density_tex_step", temp, 2);
            
            GOOF::Int3 quadrantOffset((unsigned int)index.x % (1 << lodLevel),
                                      (unsigned int)index.y % (1 << lodLevel),
                                      (unsigned int)index.z % (1 << lodLevel));
            //mSharedParams->setNamedConstant("g_lod_quadrant_offset_uvw", lodScale * quadrantOffset.toVector3() * invVoxelDimPlusMarginsMinusOne * (mVoxelDim-1));
            
            float pixelStep = (float) (1 << (mLodCount - lodLevel - 1) );
            mSharedParams->setNamedConstant("g_step_pixels", pixelStep);
            
            Ogre::Vector3 quadrantOffsetPixels = (quadrantOffset * (mVoxelDim-1)).toVector3() * pixelStep;
            mSharedParams->setNamedConstant("g_quadrant_offset_pixels", quadrantOffsetPixels);
        }

        void SharedParams::setTransitionParams(Ogre::uint8 face, Ogre::uint8 side)
        {
            static const Ogre::Vector3 TRANSITION_FACE[3] =
            {
                Ogre::Vector3::UNIT_X,
                Ogre::Vector3::UNIT_Y,
                Ogre::Vector3::UNIT_Z
            };
            
            mSharedParams->setNamedConstant("g_transition_face", TRANSITION_FACE[face]);
            mSharedParams->setNamedConstant("g_one_minus_transition_face", Ogre::Vector3::UNIT_SCALE -TRANSITION_FACE[face]);
            mSharedParams->setNamedConstant("g_transition_face_side", (float) side);
            mSharedParams->setNamedConstant("g_transition_face_idx",  (float) (face*2 + side));
        }
        
        void SharedParams::logParams(Ogre::String& log)
        {
            Ogre::GpuConstantDefinitionIterator it = mSharedParams->getConstantDefinitionIterator();
            
            while (it.hasMoreElements())
            {
                Ogre::String name = it.peekNextKey();
                const Ogre::GpuConstantDefinition& def = it.getNext();
                const float* pFloat = mSharedParams->getFloatPointer(def.physicalIndex);
                
                if(def.constType == Ogre::GCT_FLOAT1)
                {
                    log += name + " " + Ogre::StringConverter::toString(*pFloat) + "\n";
                }
                
                else if(def.constType == Ogre::GCT_FLOAT2)
                {
                    Ogre::Vector2 val;
                    val.x = *pFloat++;
                    val.y = *pFloat;
                    log += name + " " + Ogre::StringConverter::toString(val) + "\n";
                }
                
                else if(def.constType == Ogre::GCT_FLOAT3)
                {
                    Ogre::Vector3 val;
                    val.x = *pFloat++;
                    val.y = *pFloat++;
                    val.z = *pFloat;
                    log += name + " " + Ogre::StringConverter::toString(val) + "\n";
                }
            }
        }
        
        RegularSeed::RegularSeed(Ogre::SceneManager* sceneMgr, const SharedParams* sharedParams)
        {
            mSeedObject = sceneMgr->createManualObject();
            build(sharedParams);
        }
        
        RegularSeed::~RegularSeed()
        {
            mSeedObject->_getManager()->destroyManualObject(mSeedObject);
        }
        
        void RegularSeed::build(const SharedParams* sharedParams)
        {
            mSeedObject->clear();
            Ogre::uint8 voxelDimMinusOne = sharedParams->getVoxelDim() - 1;
            
            mSeedObject->estimateVertexCount(voxelDimMinusOne * voxelDimMinusOne * voxelDimMinusOne);
            mSeedObject->begin("BaseWhiteNoLighting", Ogre::RenderOperation::OT_POINT_LIST);
            
            for(int d=0; d<voxelDimMinusOne; d++)
            {
                for(int h=0; h<voxelDimMinusOne; h++)
                {
                    for(int w=0; w<voxelDimMinusOne; w++)
                    {
                        mSeedObject->position(Ogre::Vector3(w, h, d));
                        
                        // mSeedObject->textureCoord(Vector3((margin + w) * invVoxelDimPlusMarginsMinusOne, (margin + h) * invVoxelDimPlusMarginsMinusOne, (margin + d) * invVoxelDimPlusMarginsMinusOne));
                        
                    }
                }
            }
            mSeedObject->end();
        }
        
        TransitionSeed::TransitionSeed(Ogre::SceneManager* sceneMgr, const SharedParams* sharedParams)
        {
            mSeedObject = sceneMgr->createManualObject();
            build(sharedParams);
        }
        
        TransitionSeed::~TransitionSeed()
        {
            mSeedObject->_getManager()->destroyManualObject(mSeedObject);
        }
        
        void TransitionSeed::build(const SharedParams* sharedParams)
        {
            Ogre::uint8 voxelDimMinusOne = sharedParams->getVoxelDim() - 1;
            
            mSeedObject->clear();
            mSeedObject->estimateVertexCount(voxelDimMinusOne * voxelDimMinusOne);
            mSeedObject->begin("BaseWhiteNoLighting", Ogre::RenderOperation::OT_POINT_LIST);
            
            for(int h=0; h<voxelDimMinusOne; h++)
            {
                for(int w=0; w<voxelDimMinusOne; w++)
                {
                    mSeedObject->position(Ogre::Vector3(w, h, 0));
                    // mSeedObject->textureCoord(Ogre::Vector3((margin + w) * invVoxelDimPlusMarginsMinusOne, (margin + h) * invVoxelDimPlusMarginsMinusOne, 0));
                }
            }
            
            mSeedObject->end();
        }
        
        ListTriangles::ListTriangles(Ogre::SceneManager* sceneMgr, Ogre::ManualObject* seed, const SharedParams* params, const Ogre::String& materialName)
        {
            Ogre::uint numCells = params->getVoxelDim() - 1;
            
            mList = static_cast<ProceduralRenderable*>(sceneMgr->createMovableObject(ProceduralRenderableFactory::FACTORY_TYPE_NAME));
            
            // typically enough
            Ogre::uint r2vbMaxVertexCount = numCells * numCells * numCells / 4;
                        
            Ogre::RenderToVertexBufferSharedPtr r2vb = createRenderToVertexBuffer(r2vbMaxVertexCount, materialName);
            r2vb->setSourceRenderable(seed->getSection(0));
            mr2vbs.push_back(r2vb);
           
            // binds the two together
            mList->setRenderToVertexBuffer(r2vb);
        }
        
        void ListTriangles::setDensity(Ogre::TexturePtr tex)
        {
            mList->getRenderToVertexBuffer()->getRenderToBufferMaterial()->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTexture(tex);
        }
        
        void ListTriangles::update(Ogre::SceneManager* sceneMgr)
        {
            if(mList->getRenderToVertexBuffer().getPointer() != mr2vbs[0].getPointer())
                mList->setRenderToVertexBuffer(mr2vbs[0]);

            Ogre::RenderToVertexBufferSharedPtr r2vb = mList->getRenderToVertexBuffer();
            
            
            // update actually adds a stream out query so can use vertexCount
            r2vb->update(sceneMgr);
            
            Ogre::RenderOperation op;
            r2vb->getRenderOperation(op);
            
            std::vector<Ogre::RenderToVertexBufferSharedPtr>::iterator it = mr2vbs.begin()+1;
            
            // GL3PlusRenderToVertexBuffer::reallocateBuffer actually sets the vertex buffer size to r2vb->getMaxVertexCount() + 1
            while(op.vertexData->vertexCount >= r2vb->getMaxVertexCount())
            {
                if(it != mr2vbs.end())
                {
                    r2vb = *it;
                    it++;
                }
                else
                {
                    r2vb = createRenderToVertexBuffer(r2vb->getMaxVertexCount() * 2, r2vb->getRenderToBufferMaterial()->getName());
                    Ogre::Renderable* source = const_cast<Ogre::Renderable*> (mList->getRenderToVertexBuffer()->getSourceRenderable());
                    r2vb->setSourceRenderable(source);
                    mr2vbs.push_back(r2vb);
                    it = mr2vbs.end();
                }
                
                mList->setRenderToVertexBuffer(r2vb);
                
                // try again
                r2vb->update(sceneMgr);
                r2vb->getRenderOperation(op);
            }
        }
        
        size_t ListTriangles::vertexCount()
        {
            Ogre::RenderOperation op;
            mList->getRenderOperation(op);
            return op.vertexData->vertexCount;
        }
        
        Ogre::RenderToVertexBufferSharedPtr ListTriangles::createRenderToVertexBuffer( Ogre::uint r2vbMaxVertexCount, const Ogre::String& materialName )
        {
            Ogre::RenderToVertexBufferSharedPtr r2vb = Ogre::HardwareBufferManager::getSingleton().createRenderToVertexBuffer();
            r2vb->setRenderToBufferMaterialName( materialName );
            
            r2vb->setOperationType(Ogre::RenderOperation::OT_POINT_LIST);
            r2vb->setMaxVertexCount(r2vbMaxVertexCount);
            r2vb->setResetsEveryUpdate(true); // resets everytime updates called
            
            Ogre::VertexDeclaration* vertexDecl = r2vb->getVertexDeclaration();
            size_t offset = 0;
            offset += vertexDecl->addElement(0, offset, Ogre::VET_UINT1, Ogre::VES_POSITION).getSize();
            
            return r2vb;
        }
        
        void ListTriangles::logTriangle(Ogre::uint& val)
        {
            //   8-bit       12-bit      4-bit
            // cube_case    z4_y4_x4     poly4
            Ogre::uint cube_case = (val >> 16);
            Ogre::uint poly = val & 0xF;
            
            Ogre::uint pos[3];
            pos[0] = (val >> 4) & 0xF;
            pos[1] = (val >> 8) & 0xF;
            pos[2] = (val >> 12) & 0xF;
            
            Ogre::LogManager::getSingletonPtr()->logMessage(" - cube case " + Ogre::StringConverter::toString(cube_case, 3, ' ') +
                                                            " pos " + Ogre::StringConverter::toString(pos[0], 2, ' ') + " " + Ogre::StringConverter::toString(pos[1], 2, ' ') + " " + Ogre::StringConverter::toString(pos[2], 2, ' ') +
                                                            " poly " + Ogre::StringConverter::toString(poly, 2, ' '));
        }
        
        void ListTriangles::log(Ogre::uint16 max, const Ogre::String& text)
        {
            Ogre::RenderToVertexBufferSharedPtr r2vb = mList->getRenderToVertexBuffer();
            
            Ogre::RenderOperation op;
            r2vb->getRenderOperation(op);

            max = max == 0 ? op.vertexData->vertexCount : max < op.vertexData->vertexCount ? max : op.vertexData->vertexCount;
            
            Ogre::LogManager::getSingletonPtr()->logMessage(" * " + r2vb->getRenderToBufferMaterial()->getName() +
                                                            ((text == "") ? "" : "\n * " + text) +
                                                            "\n * vertex count " + Ogre::StringConverter::toString(op.vertexData->vertexCount) + " logging " + Ogre::StringConverter::toString(max) + " of them" );

            Ogre::HardwareVertexBufferSharedPtr vBuf = op.vertexData->vertexBufferBinding->getBuffer(0);
            Ogre::uint* p_uint = static_cast<Ogre::uint*>(vBuf->lock(Ogre::HardwareBuffer::HBL_READ_ONLY));
            
            for(int i=0; i<max; i++)
            {
                logTriangle(*p_uint++);
            }
            vBuf->unlock();
        }
        
        
        RegularTriTableTexture::RegularTriTableTexture()
        {
            Ogre::String mTextureName = "VT/RegularTriTableTexture";
            
            if(Ogre::TextureManager::getSingleton().resourceExists(mTextureName))
            {
                mTriTable = Ogre::TextureManager::getSingleton().getByName(mTextureName);
                return;
            }
            
            static const int EDGE_START_IND[12] =
            {
                0, 4, 1, 0, 2, 6, 3, 2, 0, 4, 5, 1,
            };
            
            static const int EDGE_END_IND[12] =
            {
                4, 5, 5, 1, 6, 7, 7, 3, 2, 6, 7, 3,
            };
            
            int width = 256 * 5; // cube case * max num triangles
            int height = 1;
            mTriTable = Ogre::TextureManager::getSingleton().createManual(mTextureName,
                                                                          Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                                                                          Ogre::TEX_TYPE_1D,
                                                                          width, height, 0, Ogre::PF_FLOAT16_RGB, Ogre::TU_DEFAULT);

            const Ogre::PixelBox& pb = mTriTable->getBuffer()->lock(Ogre::Image::Box(0, 0, width, height), Ogre::HardwareBuffer::HBL_DISCARD);
            
            // a single byte pointer
            Ogre::uint8* dest = static_cast<Ogre::uint8*>(pb.data);
            size_t pixelStep = Ogre::PixelUtil::getNumElemBytes(pb.format);
            
            for(int i=0; i<256; i++)
            {
                Ogre::uint32 tableIndex = i;
                
                Ogre::uint32 classIndex = regularCellClass[tableIndex];
                const RegularCellData* data = &regularCellData[classIndex];
                
                for (Ogre::uint16 ind = 0; ind < 15; ind+=3)
                {
                    Ogre::ColourValue cv(-1,-1,-1,-1);
                    
                    if(ind < data->GetTriangleCount()*3)
                    {
                        for(int j=0; j<3; j++)
                        {
                            Ogre::uint16 data2 = regularVertexData[tableIndex][ data->vertexIndex[ind+j] ];
                            
                            Ogre::uint16 corner0 = data2 & 0x0F;
                            Ogre::uint16 corner1 = (data2 & 0xF0) >> 4;
                            
                            for(int k=0; k<12; k++)
                            {
                                if((EDGE_START_IND[k] == corner0 && EDGE_END_IND[k] == corner1) ||
                                   (EDGE_START_IND[k] == corner1 && EDGE_END_IND[k] == corner0)
                                   )
                                {
                                    cv[j] = k;
                                }
                            }
                        }
                    }
                    
                    Ogre::PixelUtil::packColour(cv, pb.format, dest);
                    dest += pixelStep;
                }
            }
            
            mTriTable->getBuffer()->unlock();
        }
        
        GenTriangles::GenTriangles(Ogre::TexturePtr triTable, const Ogre::String& materialName)
        {
            mGenTrianglesMat = Ogre::MaterialManager::getSingleton().load(materialName, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME).staticCast<Ogre::Material>();
            mGenTrianglesMat->getTechnique(0)->getPass(0)->getTextureUnitState(1)->setTexture(triTable);
        }
        
        Ogre::RenderToVertexBufferSharedPtr GenTriangles::createRenderToVertexBuffer( Ogre::uint r2vbMaxVertexCount )
        {
            Ogre::RenderToVertexBufferSharedPtr r2vb = Ogre::HardwareBufferManager::getSingleton().createRenderToVertexBuffer();
            r2vb->setRenderToBufferMaterialName(mGenTrianglesMat->getName());
            r2vb->setOperationType(Ogre::RenderOperation::OT_TRIANGLE_LIST); // OT_TRIANGLE_LIST
            r2vb->setResetsEveryUpdate(false);
            r2vb->setMaxVertexCount(r2vbMaxVertexCount);

            Ogre::VertexDeclaration* vertexDecl = r2vb->getVertexDeclaration();
            Ogre::uint offset = 0;
            offset += vertexDecl->addElement(0, offset, Ogre::VET_FLOAT3, Ogre::VES_POSITION).getSize();
            offset += vertexDecl->addElement(0, offset, Ogre::VET_FLOAT3, Ogre::VES_TEXTURE_COORDINATES, 0).getSize();
            
            addToVertexDeclartion(vertexDecl, offset);
            
            return r2vb;
        }
        
        void GenTriangles::setDensity(Ogre::TexturePtr tex)
        {
            mGenTrianglesMat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTexture(tex);
        }
        
        void GenTriangles::update(Ogre::SceneManager* sceneMgr, ProceduralRenderable* list, ProceduralRenderable* target)
        {
            Ogre::RenderToVertexBufferSharedPtr r2vb = target->getRenderToVertexBuffer();
            
            // use approximate size
            Ogre::uint r2vbMaxVertexCount = list->getRenderToVertexBuffer()->getMaxVertexCount()*3;
            
            // use exact size
            //Ogre::RenderOperation op;
            //list->getRenderToVertexBuffer()->getRenderOperation(op);
            //Ogre::uint r2vbMaxVertexCount = op.vertexData->vertexCount * 3;
            
            if(r2vb.isNull() || r2vb->getMaxVertexCount() != r2vbMaxVertexCount)
            {
                r2vb = createRenderToVertexBuffer(r2vbMaxVertexCount);
                target->setRenderToVertexBuffer(r2vb);
            }
            
            r2vb->setSourceRenderable(list);
            r2vb->reset();
            r2vb->update(sceneMgr);
            
            // validity check recreate where fail (should NOT be necessary)
            {
                Ogre::RenderOperation tri_op;
                list->getRenderToVertexBuffer()->getRenderOperation(tri_op);
                Ogre::uint expected = tri_op.vertexData->vertexCount * 3;

                Ogre::RenderOperation op;
                r2vb->getRenderOperation(op);
                
                if( expected != op.vertexData->vertexCount )
                {
                    printf("failed expected tri count not met\n");
                    
                    update(sceneMgr, list, target);
                    return;
                }
                
                Ogre::HardwareVertexBufferSharedPtr vBuf = op.vertexData->vertexBufferBinding->getBuffer(0);
                Ogre::Real* pReal = static_cast<Ogre::Real*>(vBuf->lock(Ogre::HardwareBuffer::HBL_READ_ONLY));
                
                for(int i=0; i<op.vertexData->vertexCount; i++)
                {
                    Ogre::Vector3 pos;
                    for(int j=0; j<3; j++)
                    {
                        pos[j] = *pReal++;
                    }
                    
                    Ogre::Vector3 uv0;
                    for(int j=0; j<3; j++)
                    {
                        uv0[j] = *pReal++;
                    }
                    
                    if( pos == Ogre::Vector3::ZERO && uv0 == Ogre::Vector3::ZERO )
                    {
                        printf("error : some tri with zero pos and uv0 zero\n");
                        /*
                        vBuf->unlock();
                        update(sceneMgr, list, target);
                        return;
                        */
                    }
                }
                vBuf->unlock();
            }
        }
        
        void GenTriangles::log(ProceduralRenderable* target, Ogre::uint16 max, const Ogre::String& text)
        {
            Ogre::RenderToVertexBufferSharedPtr r2vb = target->getRenderToVertexBuffer();
            
            Ogre::RenderOperation op;
            r2vb->getRenderOperation(op);
            
            max *= 3; // num vertices
            max = max == 0 ? op.vertexData->vertexCount : max < op.vertexData->vertexCount ? max : op.vertexData->vertexCount;
            
            Ogre::LogManager::getSingletonPtr()->logMessage(" * " + r2vb->getRenderToBufferMaterial()->getName() +
                                                            ((text == "") ? "" : "\n * " + text) +
                                                            "\n * vertex count " + Ogre::StringConverter::toString(op.vertexData->vertexCount) + " logging " + Ogre::StringConverter::toString(max) + " of them" );
            
            Ogre::HardwareVertexBufferSharedPtr vBuf = op.vertexData->vertexBufferBinding->getBuffer(0);
            Ogre::Real* pReal = static_cast<Ogre::Real*>(vBuf->lock(Ogre::HardwareBuffer::HBL_READ_ONLY));
            
            for(int i=0; i<max; i++)
            {
                Ogre::Vector3 pos;
                for(int j=0; j<3; j++)
                {
                    pos[j] = *pReal++;
                }
                
                Ogre::Vector3 uv0;
                for(int j=0; j<3; j++)
                {
                    uv0[j] = *pReal++;
                }
                
                if(i%3 == 0)
                {
                    Ogre::LogManager::getSingletonPtr()->logMessage(" - triangle " + Ogre::StringConverter::toString(i/3));
                }
                
                Ogre::LogManager::getSingletonPtr()->logMessage(" - pos " + Ogre::StringConverter::toString(pos) +
                                                                " uv0 " + Ogre::StringConverter::toString(uv0));
            }
            vBuf->unlock();
        }
        
        
        GenTrianglesDelta::GenTrianglesDelta(Ogre::TexturePtr triTable, const Ogre::String& materialName) :
            GenTriangles(triTable, materialName)
        {
        }
        
        void GenTrianglesDelta::addToVertexDeclartion(Ogre::VertexDeclaration* vertexDecl, Ogre::uint& offset)
        {
            offset += vertexDecl->addElement(0, offset, Ogre::VET_FLOAT3, Ogre::VES_TEXTURE_COORDINATES, 1).getSize();
        }
        
        TransitionListTriangles::TransitionListTriangles(Ogre::SceneManager* sceneMgr, Ogre::ManualObject* seed, const SharedParams* params, const Ogre::String& materialName) :
            ListTriangles(sceneMgr, seed, params, materialName)
        {
            Ogre::uint numCells = params->getVoxelDim() - 1;
            
            // typically enough
            Ogre::uint r2vbMaxVertexCount = numCells * numCells / 4;
            
            // untill we update() the the r2vb isn't actually allocated (so this is ok)
            mList->getRenderToVertexBuffer()->setMaxVertexCount(r2vbMaxVertexCount);
        }
        
        TransitionTriTableTexture::TransitionTriTableTexture()
        {
            Ogre::String mTextureName = "VT/TransitionTriTableTexture";
            
            Ogre::uint16 EdgeStartInd[20] =
            {
                0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 6, 6, 7, 8, 9, 9, 10, 11
            };
            
            Ogre::uint16 EdgeEndInd[20] =
            {
                1, 3, 9, 2, 4, 5, 10, 4, 6, 5, 7, 8, 7, 11, 8, 12, 10, 11, 12, 12
            };
            
            
            int width = 512 * 12; // cube case * max num triangles
            int height = 1;
            mTriTable = Ogre::TextureManager::getSingleton().createManual(mTextureName,
                                                                          Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                                                                          Ogre::TEX_TYPE_1D,
                                                                          width, height, 0, Ogre::PF_FLOAT16_RGB, Ogre::TU_DEFAULT);
            
            
            const Ogre::PixelBox& pb = mTriTable->getBuffer()->lock(Ogre::Image::Box(0, 0, width, height), Ogre::HardwareBuffer::HBL_DISCARD);
            
            // a single byte pointer
            Ogre::uint8* dest = static_cast<Ogre::uint8*>(pb.data);
            size_t pixelStep = Ogre::PixelUtil::getNumElemBytes(pb.format);
            
            for(int i=0; i<512; i++)
            {
                Ogre::uint32 tableIndex = i;
                
                Ogre::uint32 classIndex = transitionCellClass[tableIndex];
                const TransitionCellData* data = &transitionCellData[classIndex & 0x7F]; // only the last 7 bit count
                
                for (Ogre::uint16 ind = 0; ind < 36; ind+=3)
                {
                    Ogre::ColourValue cv(-1,-1,-1,-1);
                    
                    if(ind < data->GetTriangleCount()*3)
                    {
                        for(int j=0; j<3; j++)
                        {
                            Ogre::uint16 data2 = transitionVertexData[tableIndex][ data->vertexIndex[ind+j] ];
                            
                            Ogre::uint16 corner0 = data2 & 0x0F;
                            Ogre::uint16 corner1 = (data2 & 0xF0) >> 4;

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
                    
                    Ogre::PixelUtil::packColour(cv, pb.format, dest);
                    dest += pixelStep;
                }
            }
            
            mTriTable->getBuffer()->unlock();
        }
        
        TransitionGenTriangles::TransitionGenTriangles(Ogre::TexturePtr triTable, const Ogre::String& materialName) :
            GenTriangles(triTable, materialName)
        {
        }
        
        RayCast::RayCast(Ogre::SceneManager* sceneMgr, const SharedParams* params, const Ogre::String& materialName)
        {
            Ogre::uint numCells = params->getVoxelDim();
            
            mHitPoints = static_cast<ProceduralRenderable*>(sceneMgr->createMovableObject(ProceduralRenderableFactory::FACTORY_TYPE_NAME));
            
            // a single ray can't really hit any more than this
            Ogre::uint r2vbMaxVertexCount = numCells;
            
            Ogre::RenderToVertexBufferSharedPtr r2vb = Ogre::HardwareBufferManager::getSingleton().createRenderToVertexBuffer();
            r2vb->setRenderToBufferMaterialName( materialName );
            r2vb->setOperationType(Ogre::RenderOperation::OT_POINT_LIST);
            r2vb->setMaxVertexCount(r2vbMaxVertexCount);
            r2vb->setResetsEveryUpdate(true); // resets everytime updates called
            
            Ogre::VertexDeclaration* vertexDecl = r2vb->getVertexDeclaration();
            size_t offset = 0;
            offset += vertexDecl->addElement(0, offset, Ogre::VET_FLOAT1, Ogre::VES_POSITION).getSize();

            // binds the two together
            mHitPoints->setRenderToVertexBuffer(r2vb);
        }
        
        void RayCast::update(Ogre::SceneManager* sceneMgr, ProceduralRenderable* triangles, const Ogre::Ray& ray, Ogre::Real& nearest)
        {
            Ogre::RenderToVertexBufferSharedPtr r2vb = mHitPoints->getRenderToVertexBuffer();
            r2vb->setSourceRenderable(triangles);
            r2vb->reset();
            
            Ogre::GpuProgramParametersSharedPtr params = r2vb->getRenderToBufferMaterial()->getTechnique(0)->getPass(0)->getGeometryProgramParameters();
            params->setNamedConstant("rayOrigin", ray.getOrigin());
            params->setNamedConstant("rayDir", ray.getDirection());
            
            // not setting a ray length (infinite ray)
            
            r2vb->update(sceneMgr);
            r2vb->setSourceRenderable(0);
            
            Ogre::RenderOperation op;
            r2vb->getRenderOperation(op);
            
            if(op.vertexData->vertexCount)
            {
                Ogre::HardwareVertexBufferSharedPtr vBuf = op.vertexData->vertexBufferBinding->getBuffer(0);
                Ogre::Real* pReal = static_cast<Ogre::Real*>(vBuf->lock(Ogre::HardwareBuffer::HBL_READ_ONLY));
                
                for(int i=0; i<op.vertexData->vertexCount; i++)
                {
                    Ogre::Real val = *pReal++;
                    
                    if(val < nearest)
                    {
                        nearest = val;
                    }
                }
                
                vBuf->unlock();
            }
        }
    }
}
