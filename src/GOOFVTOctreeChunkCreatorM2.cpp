#include "GOOFVTOctreeChunkCreatorM2.h"
#include "GOOFVTOctreeChunk.h"
#include "GOOFVTRoot.h"
#include "GOOFVTDensity.h"
#include "GOOFSlicing.h"
#include "GOOFTextureUtils.h"
#include "GOOFMaterialUtils.h"
#include "GOOFProceduralRenderable.h"

//#include "GOOFVTRegional.h"
//#include "GOOFVTMaterialBuilder.h"
//#include "GOOFPrintOgre.h"
#include "OgreWireBoundingBox.h"
#include "GOOFDebugTextureManager.h" // temp

using namespace Ogre;

namespace GOOF
{
namespace VT
{
	//-----------------------------------------------------------------------
	OctreeChunkCreatorM2::~OctreeChunkCreatorM2()
	{
		delete mDensity;
		if(mInternal_rvs)
		{
			delete mInternal_rvs;
		}

		for(std::vector<Slicing*>::iterator it = mSlicingToRecycle.begin(); it != mSlicingToRecycle.end(); ++it)
		{
			delete *it;
		}
	}

	//-----------------------------------------------------------------------
	 OctreeChunkCreatorM2::OctreeChunkCreatorM2() : mInternal_rvs(0)
	{
		VT::Root* root = VT::Root::getSingletonPtr();
		
		mDensity = new Density();

        //-----------------------------------------------------------------------
        // Create material to list triangles
        //-----------------------------------------------------------------------
		String materialName = "VT/M2ListTriangles"; // + lodIndexAsString;

		String vertProgramName = materialName + "_VPCG";
		String geomProgramName = materialName + "_GPCG";

        HighLevelGpuProgramPtr hgp = HighLevelGpuProgramManager::getSingleton().createProgram(vertProgramName,
																 ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
																 "cg",
																 GPT_VERTEX_PROGRAM);
		hgp->setSourceFile("M2ListTriangles_vp.cg.h");
		hgp->setParameter("profiles", "glslv");
		hgp->setParameter("entry_point", "main");
		hgp->load();

        Ogre::LogManager::getSingletonPtr()->logMessage("geomProgramName " + geomProgramName);
		hgp = HighLevelGpuProgramManager::getSingleton().createProgram(geomProgramName,
																 ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
																 "cg",
																 GPT_GEOMETRY_PROGRAM);
		hgp->setSourceFile("M2ListTriangles_gp.cg.h");
		hgp->setParameter("profiles", "glslg"); // gp4gp gpu_gp
		hgp->setParameter("input_operation_type", "point_list");
		hgp->setParameter("output_operation_type", "point_list");
		hgp->setParameter("max_output_vertices", "18");
		hgp->setParameter("entry_point", "main");
		hgp->load();
        
        //hgp = HighLevelGpuProgramManager::getSingleton().load("VT/M2ListTriangles_GP_GLSL", ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME).staticCast<HighLevelGpuProgram>();
        

		mListTrianglesMaterial = MaterialManager::getSingleton().create(materialName, ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);

		Pass* pass = mListTrianglesMaterial->getTechnique(0)->getPass(0);
		pass->setVertexProgram(vertProgramName);
		pass->getVertexProgramParameters()->addSharedParameters( root->getGpuSharedParameter() );
		pass->setGeometryProgram(geomProgramName);
		pass->getGeometryProgramParameters()->addSharedParameters( root->getGpuSharedParameter() );
		//root->CreateTextureUnitStates(pass, VT::Root::TT_TRI_TABLE);

		// Density volume
		TextureUnitState* tunit = pass->createTextureUnitState();
		tunit->setTextureAddressingMode(TextureUnitState::TAM_CLAMP);
		mDensityTextureRefs.push_back(tunit);
		// Note building the voxels wants point sampling BUT the AO wants bilinear filtering
		//tunit->setTextureFiltering(TFO_NONE);

		// Match named params to AutoConstantDefinition names and set where matching
		MaterialUtils::SetConstants(mListTrianglesMaterial);
        
        mListTrianglesMaterial = MaterialManager::getSingleton().load("VT/M2ListTriangles", ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME).staticCast<Material>();


        //-----------------------------------------------------------------------
        // Create materials for generating vertices
        //-----------------------------------------------------------------------
		for(int i=0; i<2; i++)
		{
			static const Ogre::String GenVerticesMaterialNames[2] = 
			{
				"VT/M2GenVertices",
				"VT/M2GenVerticesIncludeDelta"
			};
			static const Ogre::String GenVerticesCompileArgs[2] = 
			{
				"",
				" -DINCLUDE_DELTA"
			};

			materialName = GenVerticesMaterialNames[i];

			vertProgramName = materialName + "_VPCG";
			geomProgramName = materialName + "_GPCG";

			hgp = HighLevelGpuProgramManager::getSingleton().createProgram(vertProgramName,
																 ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
																 "cg",
																 GPT_VERTEX_PROGRAM);
			hgp->setSourceFile("M2GenVertices_vp.cg.h");
			hgp->setParameter("profiles", "gp4vp glslv");
			hgp->setParameter("entry_point", "main");
			hgp->setParameter("compile_arguments", GenVerticesCompileArgs[i]);
			hgp->load();

			Ogre::LogManager::getSingletonPtr()->logMessage("geomProgramName " + geomProgramName);
			hgp = HighLevelGpuProgramManager::getSingleton().createProgram(geomProgramName,
																 ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
																 "cg",
																 GPT_GEOMETRY_PROGRAM);
			hgp->setSourceFile("M2GenVertices_gp.cg.h");
			hgp->setParameter("profiles", "gp4gp gpu_gp");
			hgp->setParameter("entry_point", "main");
			hgp->setParameter("compile_arguments", GenVerticesCompileArgs[i]);
			hgp->load();

			mGenVerticesMaterial[i] = MaterialManager::getSingleton().create(materialName, ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);

			pass = mGenVerticesMaterial[i]->getTechnique(0)->getPass(0);
			pass->setVertexProgram(vertProgramName);
			pass->getVertexProgramParameters()->addSharedParameters( root->getGpuSharedParameter() );
			pass->setGeometryProgram(geomProgramName);
			pass->getGeometryProgramParameters()->addSharedParameters( root->getGpuSharedParameter() );

			root->CreateTextureUnitStates(pass, VT::Root::TT_TRI_TABLE);

			// Density volume
			tunit = pass->createTextureUnitState();
			tunit->setTextureAddressingMode(TextureUnitState::TAM_CLAMP);
			mDensityTextureRefs.push_back(tunit);
			// Note building the voxels wants point sampling BUT the AO wants bilinear filtering
			//tunit->setTextureFiltering(TFO_NONE);

			// Match named params to AutoConstantDefinition names and set where matching
			MaterialUtils::SetConstants(mGenVerticesMaterial[i]);
		}

		//MaterialSerializer serializer;
		//serializer.queueForExport(mListTrianglesMaterial);
		//serializer.queueForExport(mGenVerticesMaterial[0]);
		//const Ogre::String materalContents = serializer.getQueuedAsString();
		//Ogre::LogManager::getSingletonPtr()->logMessage(materalContents);


		//-----------------------------------------------------------------------
		// List vertices RenderToVertexBuffer (reuse this object)
		//-----------------------------------------------------------------------
        Ogre::uint numCells = root->getVoxelDim() - 1;

		Ogre::uint r2vbMaxVertexCount = numCells * numCells * numCells; // Excessive

        Ogre::RenderToVertexBufferSharedPtr r2vblistTri = HardwareBufferManager::getSingleton().createRenderToVertexBuffer();

        // Set the type of primitives that this object generates.
		r2vblistTri->setOperationType(RenderOperation::OT_POINT_LIST);
		r2vblistTri->setMaxVertexCount(r2vbMaxVertexCount);
		r2vblistTri->setResetsEveryUpdate(false);

		Ogre::VertexDeclaration* vertexDecl = r2vblistTri->getVertexDeclaration();
		size_t offset = 0;
		offset += vertexDecl->addElement(0, offset, VET_FLOAT1, VES_TEXTURE_COORDINATES, 0).getSize();

        r2vblistTri->setSourceRenderable(root->getSeedObject()->getSection(0));
        r2vblistTri->setRenderToBufferMaterialName(mListTrianglesMaterial->getName());

        // So we can use the data generated in listTriices as input to the GenVertices
        // (ProceduralRenderable is derived from Ogre::Renderable)
		mListTriObject = static_cast<ProceduralRenderable*> (getSceneManager()->createMovableObject("ListTriObject", ProceduralRenderableFactory::FACTORY_TYPE_NAME));
        //mListTriObject = new GOOF::ProceduralRenderable();
        mListTriObject->setRenderToVertexBuffer(r2vblistTri);


		//-----------------------------------------------------------------------
        // Create material for transition list triangles
        //-----------------------------------------------------------------------
		materialName = "VT/M2TransitionListTriangles"; // + lodIndexAsString;

		vertProgramName = materialName + "_VPCG";
		geomProgramName = materialName + "_GPCG";

		hgp = HighLevelGpuProgramManager::getSingleton().createProgram(vertProgramName,
																	   ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
																	   "cg",
																	   GPT_VERTEX_PROGRAM);
		hgp->setSourceFile("M2TransitionListTriangles_vp.cg.h");
		hgp->setParameter("profiles", "gp4vp glslv");
		hgp->setParameter("entry_point", "main");
		hgp->load();

        Ogre::LogManager::getSingletonPtr()->logMessage("geomProgramName " + geomProgramName);
		hgp = HighLevelGpuProgramManager::getSingleton().createProgram(geomProgramName,
																 ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
																 "cg",
																 GPT_GEOMETRY_PROGRAM);
		hgp->setSourceFile("M2TransitionListTriangles_gp.cg.h");
        /*
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
		hgp->setParameter("profiles", "glslg");
		hgp->setParameter("input_operation_type", "point_list");
		hgp->setParameter("output_operation_type", "point_list");
		hgp->setParameter("max_output_vertices", "18");
#else
		hgp->setParameter("profiles", "gp4gp gpu_gp");
#endif
        */
        hgp->setParameter("profiles", "gp4gp gpu_gp");
		hgp->setParameter("entry_point", "main");
		hgp->load();

		mTransitionListTrianglesMaterial = MaterialManager::getSingleton().create(materialName, ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
		pass = mTransitionListTrianglesMaterial->getTechnique(0)->getPass(0);
		pass->setVertexProgram(vertProgramName);
		pass->getVertexProgramParameters()->addSharedParameters( root->getGpuSharedParameter() );
		pass->setGeometryProgram(geomProgramName);
		pass->getGeometryProgramParameters()->addSharedParameters( root->getGpuSharedParameter() );
		//root->CreateTextureUnitStates(pass, VT::Root::TT_TRI_TABLE);

		// Density volume
		tunit = pass->createTextureUnitState();
		tunit->setTextureAddressingMode(TextureUnitState::TAM_CLAMP);
		mDensityTextureRefs.push_back(tunit);
		// Note building the voxels wants point sampling BUT the AO wants bilinear filtering
		//tunit->setTextureFiltering(TFO_NONE);

		// Match named params to AutoConstantDefinition names and set where matching
		MaterialUtils::SetConstants(mTransitionListTrianglesMaterial);


		
		//-----------------------------------------------------------------------
        // Create material for transition gen vertices
        //-----------------------------------------------------------------------
        materialName = "VT/M2TransitionGenVertices";

        vertProgramName = materialName + "_VPCG";
        geomProgramName = materialName + "_GPCG";

        hgp = HighLevelGpuProgramManager::getSingleton().createProgram(vertProgramName,
																 ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
																 "cg",
																 GPT_VERTEX_PROGRAM);
		hgp->setSourceFile("M2TransitionGenVertices_vp.cg.h");
		hgp->setParameter("profiles", "gp4vp");
		hgp->setParameter("entry_point", "main");
		hgp->load();

		Ogre::LogManager::getSingletonPtr()->logMessage("geomProgramName " + geomProgramName);
		hgp = HighLevelGpuProgramManager::getSingleton().createProgram(geomProgramName,
																 ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
																 "cg",
																 GPT_GEOMETRY_PROGRAM);
		hgp->setSourceFile("M2TransitionGenVertices_gp.cg.h");
		hgp->setParameter("profiles", "gp4gp gpu_gp");
		hgp->setParameter("entry_point", "main");
		hgp->load();

		mTransitionGenVerticesMaterial = MaterialManager::getSingleton().create(materialName, ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);

		pass = mTransitionGenVerticesMaterial->getTechnique(0)->getPass(0);
		pass->setVertexProgram(vertProgramName);
		pass->getVertexProgramParameters()->addSharedParameters( root->getGpuSharedParameter() );
		pass->setGeometryProgram(geomProgramName);
		pass->getGeometryProgramParameters()->addSharedParameters( root->getGpuSharedParameter() );

		root->CreateTextureUnitStates(pass, VT::Root::TT_TRANSITION_TRI_TABLE);

		// Density volume
		tunit = pass->createTextureUnitState();
		tunit->setTextureAddressingMode(TextureUnitState::TAM_CLAMP);
		mDensityTextureRefs.push_back(tunit);
		// Note building the voxels wants point sampling BUT the AO wants bilinear filtering
		//tunit->setTextureFiltering(TFO_NONE);

		// Match named params to AutoConstantDefinition names and set where matching
		MaterialUtils::SetConstants(mTransitionGenVerticesMaterial);


		//-----------------------------------------------------------------------
		// Transition list vertices RenderToVertexBuffer (reuse this object)
		//-----------------------------------------------------------------------
		r2vbMaxVertexCount = numCells * numCells; // Excessive

        r2vblistTri = HardwareBufferManager::getSingleton().createRenderToVertexBuffer();

        // Set the type of primitives that this object generates.
		r2vblistTri->setOperationType(RenderOperation::OT_POINT_LIST);
		r2vblistTri->setMaxVertexCount(r2vbMaxVertexCount);
		r2vblistTri->setResetsEveryUpdate(false);

		vertexDecl = r2vblistTri->getVertexDeclaration();
		offset = 0;
		offset += vertexDecl->addElement(0, offset, VET_FLOAT1, VES_TEXTURE_COORDINATES, 0).getSize();

        r2vblistTri->setSourceRenderable(root->getTransitionSeedObject()->getSection(0));
        r2vblistTri->setRenderToBufferMaterialName(mTransitionListTrianglesMaterial->getName());

        // So we can use the data generated in listTriices as input to the GenVertices
        // (ProceduralRenderable is derived from Ogre::Renderable)
		mTransitionListTriObject = static_cast<ProceduralRenderable*> (getSceneManager()->createMovableObject("TransitionListTriObject", ProceduralRenderableFactory::FACTORY_TYPE_NAME));
		mTransitionListTriObject->setRenderToVertexBuffer(r2vblistTri);
	}

	//-----------------------------------------------------------------------
	void OctreeChunkCreatorM2::changeDensityTexture(const Ogre::TexturePtr& texPtr)
	{
		std::vector<Ogre::TextureUnitState*>::iterator it = mDensityTextureRefs.begin();
		if((*it)->_getTexturePtr() != texPtr)
		{
			for(it = mDensityTextureRefs.begin(); it != mDensityTextureRefs.end(); ++it)
			{
				(*it)->setTexture(texPtr);
			}
		}
	}

	//-----------------------------------------------------------------------
	void OctreeChunkCreatorM2::reload()
	{
		delete mDensity;
		mDensity = new Density();
	}

	//-----------------------------------------------------------------------
    void OctreeChunkCreatorM2::print_case_z4_y4_x4_poly4(Ogre::uint& val)
    {
        Ogre::uint cube_case = (val >> 16);
        Ogre::uint poly = val & 0xF;

        Ogre::uint pos[3];
        pos[0] = (val >> 4) & 0xF;
        pos[1] = (val >> 8) & 0xF;
        pos[2] = (val >> 12) & 0xF;

        Ogre::LogManager::getSingletonPtr()->logMessage("cube case " + Ogre::StringConverter::toString(cube_case) + " poly " + Ogre::StringConverter::toString(poly));
		Ogre::LogManager::getSingletonPtr()->logMessage("pos " + Ogre::StringConverter::toString(pos[0]) + " " + Ogre::StringConverter::toString(pos[1]) + " " + Ogre::StringConverter::toString(pos[2]));
    }

    
	//-----------------------------------------------------------------------
    void OctreeChunkCreatorM2::print_z6_y6_x6_edge1_edge2_edge3(Ogre::uint& val)
    {
        // the position of the cell and the 3 edges needed
        // to place vertices on to make the triangle

        Ogre::uint pos[3];
        pos[0] = (val >> 14) & 0x3F;
        pos[1] = (val >> 20) & 0x3F;
        pos[2] = (val >> 26) & 0x3F;

		Ogre::LogManager::getSingletonPtr()->logMessage("pos " + Ogre::StringConverter::toString(pos[0]) + " " + Ogre::StringConverter::toString(pos[1]) + " " + Ogre::StringConverter::toString(pos[2]));

        pos[2] = val & 0x0F;
        pos[1] = (val >> 4) & 0x0F;
        pos[0] = (val >> 8) & 0x0F;

		Ogre::LogManager::getSingletonPtr()->logMessage("edge " + Ogre::StringConverter::toString(pos[0]) + " " + Ogre::StringConverter::toString(pos[1]) + " " + Ogre::StringConverter::toString(pos[2]));
    }
    
 	//-----------------------------------------------------------------------
    void OctreeChunkCreatorM2::print_z8_y8_x8_case8(Ogre::uint& val)
    {
        Ogre::uint cube_case = (val & 0xFF);

        Ogre::uint pos[3];
        pos[0] = (val >> 8) & 0xFF;
        pos[1] = (val >> 16) & 0xFF;
        pos[2] = (val >> 24) & 0xFF;

		Ogre::LogManager::getSingletonPtr()->logMessage("cube case " + Ogre::StringConverter::toString(cube_case));
		Ogre::LogManager::getSingletonPtr()->logMessage("pos " + Ogre::StringConverter::toString(pos[0]) + " " + Ogre::StringConverter::toString(pos[1]) + " " + Ogre::StringConverter::toString(pos[2]));
    }

 	//-----------------------------------------------------------------------
	void OctreeChunkCreatorM2::print_listTriangles(Ogre::RenderToVertexBufferSharedPtr& r2vb, const Ogre::String& title, Ogre::uint16 count)
	{
		RenderOperation op;
		r2vb->getRenderOperation(op);
		Ogre::LogManager::getSingletonPtr()->logMessage(title + " ListTri output vertexCount " + Ogre::StringConverter::toString(op.vertexData->vertexCount) );
		if(op.vertexData->vertexCount != 0)
		{
			HardwareVertexBufferSharedPtr vBuf = op.vertexData->vertexBufferBinding->getBuffer(0);
            Ogre::Real* pReal = static_cast<Ogre::Real*>(vBuf->lock(HardwareBuffer::HBL_READ_ONLY));

			count = op.vertexData->vertexCount < count ? op.vertexData->vertexCount : count;
            for(int i=0; i<count; i++)
            {
                Ogre::uint val = *pReal++;
                print_case_z4_y4_x4_poly4(val);
            }
            vBuf->unlock();
		}
	}

 	//-----------------------------------------------------------------------
	void OctreeChunkCreatorM2::print_genVertices(Ogre::RenderToVertexBufferSharedPtr& r2vb, const Ogre::String& title, bool showPos, bool showTex, Ogre::uint16 count)
	{
		RenderOperation op;
		r2vb->getRenderOperation(op);
		Ogre::LogManager::getSingletonPtr()->logMessage(title + " GenVert output vertexCount " + Ogre::StringConverter::toString(op.vertexData->vertexCount) );
        if(op.vertexData->vertexCount != 0)
		{
			HardwareVertexBufferSharedPtr vBuf = op.vertexData->vertexBufferBinding->getBuffer(0);
            Real* pReal = static_cast<Real*>(vBuf->lock(HardwareBuffer::HBL_READ_ONLY));
			
			count = op.vertexData->vertexCount < count ? op.vertexData->vertexCount : count;
            for(int i=0; i<count; i++)
            {
               // Ogre::uint val = *pReal++;
                //print_z8_y8_x8_case8(val);
                //print_z6_y6_x6_edge1_edge2_edge3(val);
                //print_z6_y6_x6_case8_poly6(val);

				Ogre::String temp;
				for(int j=0; j<4; j++)
				{
					Ogre::Real val = *pReal++;
					temp += Ogre::StringConverter::toString(val) + " ";
				}
				if(showPos)
					Ogre::LogManager::getSingletonPtr()->logMessage("pos " + temp);

				temp = "";
				for(int j=0; j<4; j++)
				{
					Ogre::Real val = *pReal++;
					temp += Ogre::StringConverter::toString(val) + " ";
				}
				if(showTex)
					Ogre::LogManager::getSingletonPtr()->logMessage("tex " + temp);
            }
            vBuf->unlock();
		}
	}



	//-----------------------------------------------------------------------
	void OctreeChunkCreatorM2::adjustSharedParamters(const Int3& index, Ogre::uint8 lodLevel, Ogre::Vector3 chunkSize)
	{
		Root* root = Root::getSingletonPtr();

		GOOF::Int3 quadrantOffset(
			(unsigned int)index.x % (1 << lodLevel),
			(unsigned int)index.y % (1 << lodLevel),	
			(unsigned int)index.z % (1 << lodLevel));

		Ogre::GpuSharedParametersPtr sharedParams = root->getGpuSharedParameter();

		sharedParams->setNamedConstant("g_world_space_chunk_pos", index.toVector3() * chunkSize);

		/*
		sharedParams->setNamedConstant("g_world_space_chunk_size", chunkSize.x);
		sharedParams->setNamedConstant("g_world_space_voxel_size", root->getLodVoxelSize(lodLevel).x);
		*/
		sharedParams->setNamedConstant("g_world_space_chunk_size", chunkSize);
		sharedParams->setNamedConstant("g_world_space_voxel_size", root->getLodVoxelSize(lodLevel));
		
		sharedParams->setNamedConstant("g_lod_index", (float) lodLevel);

		float lodScale = (float) 1.0 / (1 << lodLevel);
		sharedParams->setNamedConstant("g_lod_scale", lodScale);
		sharedParams->setNamedConstant("g_lod_quadrant_offset_uvw", root->getLodQuadrantOffset_uwv(lodLevel, quadrantOffset));
		
		float temp[2] = { 0, 0 };
		float invVoxelDimPlusMarginsMinusOne = 1.0 / float(root->getVoxelDim() + 2* root->getMargin());
		temp[0] = lodScale * invVoxelDimPlusMarginsMinusOne;
        sharedParams->setNamedConstant("g_sampler_density_tex_step", temp, 2);


		/*
		Ogre::LogManager::getSingletonPtr()->logMessage("g_world_space_chunk_pos " + Ogre::StringConverter::toString( index.toVector3() * chunkSize ));
		Ogre::LogManager::getSingletonPtr()->logMessage("g_chunkSize " + Ogre::StringConverter::toString( chunkSize));
		Ogre::LogManager::getSingletonPtr()->logMessage("g_index " +
			Ogre::StringConverter::toString(index.x) + " " + 
			Ogre::StringConverter::toString(index.y) + " " + 
			Ogre::StringConverter::toString(index.z));
		Ogre::LogManager::getSingletonPtr()->logMessage("g_lod_scale " + Ogre::StringConverter::toString( (float) 1 / (1 << lodLevel)));
		Ogre::Vector3 lodQuadrantOffset_uwv = root->getLodQuadrantOffset_uwv(lodLevel, quadrantOffset);
		Ogre::LogManager::getSingletonPtr()->logMessage("g_lod_quadrant_offset_uvw " + Ogre::StringConverter::toString(lodQuadrantOffset_uwv));
		*/
	}

	//-----------------------------------------------------------------------
	OctreeChunk* OctreeChunkCreatorM2::createRecursive(const Int3& index, OctreeChunk* parent, bool rvsStore, bool createTransitions, Ogre::uint8 quadrant, bool streamOutQuery)
    {
		if(!createTransitions)
			rvsStore = true;

        // only store rvs if transitions not created, otherwise no need to store rvs 
        OctreeChunk* chunk = create(index, parent, rvsStore, quadrant, streamOutQuery);
        
		if(chunk)
		{
			bool isHighestLod = chunk->getLodLevel() == (VT::Root::getSingleton().getLodCount() - 1) ? true : false;
        
			if(!isHighestLod)
			{
				if(createTransitions)
				{
					for(int face=0; face<3; face++)
					{
						for(int side=0; side<2; side++)
						{
							createTransition(chunk, face, side, false, streamOutQuery);
						}
					}
				}

				for(Ogre::uint8 i=0; i<8; i++)
				{
					createRecursive(index * 2 + OctreeChunk::msQuadrant[i], chunk, rvsStore, createTransitions, i, streamOutQuery);
				}
			}
		}

		return chunk;
    }

 	//-----------------------------------------------------------------------
	Slicing* OctreeChunkCreatorM2::createOrRetrieveSlicingVolume(OctreeChunk* parent, bool rvsStore)
	{
		Slicing* rvs = mInternal_rvs;
		if(!parent)
		{
			if(!mInternal_rvs)
			{
				if(mSlicingToRecycle.size())
				{
					mInternal_rvs = mSlicingToRecycle.back();
					mSlicingToRecycle.pop_back();
				}
				else
				{
					// Ogre::PF_FLOAT16_R or Ogre::PF_R8 (needs range compression in shader)
					mInternal_rvs = new Slicing(getCamera(), Ogre::PF_R8, Root::getSingleton().getRenderVolumeSize()); 
				}
			}
			rvs = mInternal_rvs;
			mDensity->render(getSceneManager(), rvs);
		}
		else if(rvsStore)
		{
			rvs = parent->rvs;
		}

		return rvs;
	}

 	//-----------------------------------------------------------------------
	OctreeChunk* OctreeChunkCreatorM2::create(const Int3& index, OctreeChunk* parent, bool rvsStore, Ogre::uint8 quadrant, bool streamOutQuery)
	{
		if(parent)
		{
			parent->mChildNeedsCreation[quadrant] = false;
		}

		Root* root = Root::getSingletonPtr();

		Ogre::uint8 lodLevel = parent != NULL ? parent->getLodLevel()+1 : 0;
		Ogre::Vector3 chunkSize = root->getLodChunkSize(lodLevel);
		adjustSharedParamters(index, lodLevel, chunkSize);

		//Ogre::LogManager::getSingletonPtr()->logMessage("Create Index " + Ogre::StringConverter::toString(index.x) + " " + Ogre::StringConverter::toString(index.y) +  " " + Ogre::StringConverter::toString(index.z) + " at lod level " + Ogre::StringConverter::toString(lodLevel));

		//-----------------------------------------------------------------------
		// Render volumes 
		// slow due to the time taken to create the resource
		//-----------------------------------------------------------------------
		//unsigned long t0 = Ogre::Root::getSingleton().getTimer()->getMilliseconds();
		//Ogre::LogManager::getSingletonPtr()->logMessage("delta " + Ogre::StringConverter::toString( Ogre::Root::getSingleton().getTimer()->getMilliseconds()-t0 ));


		Slicing* rvs = createOrRetrieveSlicingVolume(parent, rvsStore);
		changeDensityTexture(rvs->getRenderVolume());
		//GOOF::DebugTextureManager::getSingleton().addTexture(rvs->getRenderVolume(), "ScrollTexture3D");

        
		//-----------------------------------------------------------------------
		// List vertices
		//-----------------------------------------------------------------------
		Ogre::RenderToVertexBufferSharedPtr r2vbListTri = mListTriObject->getRenderToVertexBuffer();
		r2vbListTri->reset();
		// note : update actually adds a stream out query so can use vertexCount
		r2vbListTri->update(getSceneManager());


        

		//-----------------------------------------------------------------------
		// Handle empty chunks
		//-----------------------------------------------------------------------
		AxisAlignedBox aab;

		if(!queryRegular(r2vbListTri, index, parent, quadrant, chunkSize, aab))
		{
			return NULL;
		}

		//Ogre::LogManager::getSingletonPtr()->logMessage("create chunk min " + StringConverter::toString(aab.getMinimum()) +  " max " + StringConverter::toString(aab.getMaximum()));

		/*
		if(streamOutQuery)
		{
			RenderOperation op;
			r2vbListTri->getRenderOperation(op);

			if(op.vertexData->vertexCount == 0)
			{
				return NULL;
			}
		}
		*/

		//print_listTriangles(r2vbListTri, "ordinary");
		
 		//-----------------------------------------------------------------------
        // The chunk
		//-----------------------------------------------------------------------
        OctreeChunk* chunk = new OctreeChunk(this, index, lodLevel);
		if(rvsStore)
		{
			chunk->rvs = rvs;

			// forces the creation of a new render volume for the next root type chunk
			mInternal_rvs = 0; 
		}

		//AxisAlignedBox aab = getRegularBoundingBox(chunk, chunkSize);
		//AxisAlignedBox aab = getRegularBoundingBox(r2vbListTri, chunk, chunkSize);	// accurate but expensive 
	

		chunk->setBoundingBox(aab);

		//aab.setExtents(aab.getMinimum() - root->getLodVoxelSize(lodLevel),
		//			   aab.getMaximum() + root->getLodVoxelSize(lodLevel));

		//chunk->setWorldBoundingBoxPadded(aab);

		//if(lodLevel == 1)
		//{

			//Ogre::Vector3 wsChunkPos = index.toVector3() * chunkSize;
			//aab = AxisAlignedBox(wsChunkPos,  wsChunkPos + chunkSize);

			//Ogre::WireBoundingBox* wirebox = new Ogre::WireBoundingBox();
			//wirebox->setupBoundingBox(aab);
			//getSceneNode()->attachObject(wirebox);
		//}


        
		//-----------------------------------------------------------------------
		// Generate vertices
		//-----------------------------------------------------------------------
		bool isHighestLod = lodLevel == (root->getLodCount() - 1) ? true : false;

        Ogre::uint voxelDim = root->getVoxelDim();

        // Excessive
		Ogre::uint r2vbMaxVertexCount = voxelDim * voxelDim * voxelDim;

		// Generate the RenderToBufferObject
		Ogre::RenderToVertexBufferSharedPtr r2vbGenVert = HardwareBufferManager::getSingleton().createRenderToVertexBuffer();

        // Set the type of primitives that this object generates.
		r2vbGenVert->setOperationType(RenderOperation::OT_TRIANGLE_LIST); // OT_TRIANGLE_LIST
		r2vbGenVert->setMaxVertexCount(r2vbMaxVertexCount);
		r2vbGenVert->setResetsEveryUpdate(false);

		VertexDeclaration* vertexDecl = r2vbGenVert->getVertexDeclaration();
		Ogre::uint offset = 0;

		
		// position and .w = occlusion
		offset += vertexDecl->addElement(0, offset, VET_FLOAT3, VES_POSITION).getSize();
		// normal and .w = general slope
		offset += vertexDecl->addElement(0, offset, VET_FLOAT3, VES_TEXTURE_COORDINATES, 0).getSize();
		if(!isHighestLod)
		{
			offset += vertexDecl->addElement(0, offset, VET_FLOAT3, VES_TEXTURE_COORDINATES, 1).getSize();
		}

		// Bind the two together
		chunk->setRenderToVertexBuffer(r2vbGenVert);
		r2vbGenVert->setSourceRenderable(mListTriObject);
		r2vbGenVert->setRenderToBufferMaterialName(mGenVerticesMaterial[!isHighestLod]->getName());

		// Generate geometry
		r2vbGenVert->reset();
		r2vbGenVert->update(getSceneManager());
		r2vbGenVert->setSourceRenderable(0);
		//print_genVertices(r2vbGenVert, "ordinary");


		//chunk->getParentSceneNode()->_updateBounds();	// do I need to do this?
		chunk->setVisible(false);
        // temp
		if(!isHighestLod)
			chunk->setMaterial("VT/DrawIncludeDelta");
		else
			chunk->setMaterial("VT/Draw");


		
		const bool showBounds = false;

		if(!showBounds)
		{
			getSceneNode()->attachObject(chunk);
		}
		else
		{
			SceneNode::ChildNodeIterator it = getSceneNode()->getChildIterator();
			while (it.hasMoreElements())
			{
				SceneNode* child = (SceneNode*) it.getNext();
				if(child->numAttachedObjects() == 0)
				{
					child->attachObject(chunk);
				}
			}
			if(chunk->getParentSceneNode() == NULL)
			{
				SceneNode* child = getSceneNode()->createChildSceneNode();
				child->attachObject(chunk);
				child->showBoundingBox(true);
			}
		}

		if(parent)
		{
			parent->addChild(chunk, quadrant);
		}

		// associate siblings for all but the highest lod level
		if(!isHighestLod)
		{
			associateSiblings(chunk, lodLevel);
		}

		Ogre::UserObjectBindings& uob = static_cast<Ogre::MovableObject*>(chunk)->getUserObjectBindings();
		uob.setUserAny( typeid(OctreeChunk).name() , Ogre::Any(chunk));

		return chunk;
	}

	//-----------------------------------------------------------------------
	ProceduralRenderable* OctreeChunkCreatorM2::createTransition(OctreeChunk* chunk, Ogre::uint8 face, Ogre::uint8 side, bool changeShaderParams, bool streamOutQuery)
	{
		VT::Root* root = VT::Root::getSingletonPtr();

		if(changeShaderParams)
		{
			Ogre::Vector3 chunkSize = root->getLodChunkSize(chunk->getLodLevel());
			adjustSharedParamters(chunk->getIndex(), chunk->getLodLevel(), chunkSize);
			changeDensityTexture(chunk->rvs->getRenderVolume());
		}

		Ogre::uint8 transitionIdx = face*2 + side;
		chunk->mTransitionNeedsCreation[transitionIdx] = false;

		Ogre::GpuSharedParametersPtr sharedParams = root->getGpuSharedParameter();
		sharedParams->setNamedConstant("g_transition_face", OctreeChunk::msTransitionFace[face]);
		sharedParams->setNamedConstant("g_one_minus_transition_face", OctreeChunk::msOneMinusTransitionFace[face]);
		sharedParams->setNamedConstant("g_transition_face_side", (float) side);
		sharedParams->setNamedConstant("g_transition_face_idx", (float) (face*2 + side));

		//Ogre::LogManager::getSingletonPtr()->logMessage("transition_face " + Ogre::StringConverter::toString( TRANSITION_FACE[face] ));
		//Ogre::LogManager::getSingletonPtr()->logMessage("one_minus_transition_face " + Ogre::StringConverter::toString( ONE_MINUS_TRANSITION_FACE[face] ));
		//Ogre::LogManager::getSingletonPtr()->logMessage("transition_face_side " + Ogre::StringConverter::toString( side ));
		//Ogre::LogManager::getSingletonPtr()->logMessage("transition_face_idx " + Ogre::StringConverter::toString( face*2 + side ));

		//-----------------------------------------------------------------------
		// Transition List vertices
		//-----------------------------------------------------------------------
		Ogre::RenderToVertexBufferSharedPtr r2vblistTri = mTransitionListTriObject->getRenderToVertexBuffer();
		r2vblistTri->reset();
		// note : update actually adds a stream out query so can use vertexCount
		r2vblistTri->update(getSceneManager());

		/*
		//-----------------------------------------------------------------------
		// Handle empty transition
		//-----------------------------------------------------------------------
		if(streamOutQuery)
		{
			RenderOperation op;
			r2vblistTri->getRenderOperation(op);

			if(op.vertexData->vertexCount == 0)
			{
				return NULL;
			}
		}
		*/

		//print_listTriangles(r2vblistTri, "transition");

		ProceduralRenderable* transition = static_cast<ProceduralRenderable*> (getSceneManager()->createMovableObject(ProceduralRenderableFactory::FACTORY_TYPE_NAME));

		Ogre::uint r2vbMaxVertexCount = root->getVoxelDim() * root->getVoxelDim() * 3; // should be big enough

		// Generate the RenderToBufferObject
		Ogre::RenderToVertexBufferSharedPtr r2vbGenVert = HardwareBufferManager::getSingleton().createRenderToVertexBuffer();

		// Set the type of primitives that this object generates.
		r2vbGenVert->setOperationType(RenderOperation::OT_TRIANGLE_LIST); // OT_TRIANGLE_LIST
		r2vbGenVert->setMaxVertexCount(r2vbMaxVertexCount);
		r2vbGenVert->setResetsEveryUpdate(false);

		Ogre::VertexDeclaration* vertexDecl = r2vbGenVert->getVertexDeclaration();
		Ogre::uint offset = 0;
		// position and .w = occlusion
		offset += vertexDecl->addElement(0, offset, VET_FLOAT4, VES_POSITION).getSize();
		// normal and .w = general slope
		offset += vertexDecl->addElement(0, offset, VET_FLOAT4, VES_TEXTURE_COORDINATES, 0).getSize();

		// Bind the two together
		transition->setRenderToVertexBuffer(r2vbGenVert);
		r2vbGenVert->setSourceRenderable(mTransitionListTriObject);
		r2vbGenVert->setRenderToBufferMaterialName(mTransitionGenVerticesMaterial->getName());	// Need to change this later
		
		// Generate geometry
		r2vbGenVert->reset();
		r2vbGenVert->update(getSceneManager());
		r2vbGenVert->setSourceRenderable(0);

		//print_genVertices(r2vbGenVert, "transition", true, false, 20);

		chunk->setTransition(transition, transitionIdx);
		transition->setMaterial("VT/Draw");
		Ogre::AxisAlignedBox aab = getTransitionBoundingBox(chunk, root->getLodChunkSize(chunk->getLodLevel()), face, side);
		transition->setBoundingBox(aab);
		transition->setVisible(false);
		chunk->getParentSceneNode()->attachObject(transition);

		return transition;
	}

	//-----------------------------------------------------------------------
	bool OctreeChunkCreatorM2::queryRegular(Ogre::RenderToVertexBufferSharedPtr& r2vbListTri, const Int3& index, OctreeChunk* parent, Ogre::uint8 quadrant, const Ogre::Vector3& chunkSize, Ogre::AxisAlignedBox& aab)
	{
		RenderOperation op;
		r2vbListTri->getRenderOperation(op);

		if(op.vertexData->vertexCount != 0)
		{
			HardwareVertexBufferSharedPtr vBuf = op.vertexData->vertexBufferBinding->getBuffer(0);
            Ogre::Real* pReal = static_cast<Ogre::Real*>(vBuf->lock(HardwareBuffer::HBL_READ_ONLY));

			Ogre::uint val = *pReal++;
			Int3 min((val >> 4) & 0xF, (val >> 8) & 0xF, (val >> 12) & 0xF);
			Int3 max = min;
            for(int i=1; i<op.vertexData->vertexCount; i++)
            {
				Ogre::uint val = *pReal++;
				Ogre::uint pos[3];
				pos[0] = (val >> 4) & 0xF;
				pos[1] = (val >> 8) & 0xF;
				pos[2] = (val >> 12) & 0xF;

				for(int i=0; i<3; i++)
				{
					if(min[i] > pos[i])
					{
						min[i] = pos[i];
					}
					if(max[i] < pos[i])
					{
						max[i] = pos[i];
					}
				}
            }
            vBuf->unlock();

			Ogre::Vector3 wsChunkPos = index.toVector3() * chunkSize;
			max = max + 1;
			aab.setExtents(wsChunkPos + min.toVector3() * VT::Root::getSingleton().getInvVoxelDimMinusOne() * chunkSize, 
						   wsChunkPos + max.toVector3() * VT::Root::getSingleton().getInvVoxelDimMinusOne() * chunkSize);

			return true;
		}
		else
		{
			return false;
		}
	}

	/*
	//-----------------------------------------------------------------------
	Ogre::AxisAlignedBox OctreeChunkCreatorM2::getRegularBoundingBox(Ogre::RenderToVertexBufferSharedPtr& r2vblistTri, OctreeChunk* chunk, Ogre::Real chunkSize)
	{
		RenderOperation op;
		r2vblistTri->getRenderOperation(op);
		

		if(op.vertexData->vertexCount != 0)
		{
			HardwareVertexBufferSharedPtr vBuf = op.vertexData->vertexBufferBinding->getBuffer(0);
            Ogre::Real* pReal = static_cast<Ogre::Real*>(vBuf->lock(HardwareBuffer::HBL_READ_ONLY));

			Ogre::uint val = *pReal++;
			Int3 min((val >> 4) & 0xF, (val >> 8) & 0xF, (val >> 12) & 0xF);
			Int3 max = min;
            for(int i=1; i<op.vertexData->vertexCount; i++)
            {
				Ogre::uint val = *pReal++;
				Ogre::uint pos[3];
				pos[0] = (val >> 4) & 0xF;
				pos[1] = (val >> 8) & 0xF;
				pos[2] = (val >> 12) & 0xF;

				for(int i=0; i<3; i++)
				{
					if(min[i] > pos[i])
					{
						min[i] = pos[i];
					}
					if(max[i] < pos[i])
					{
						max[i] = pos[i];
					}
				}
            }
            vBuf->unlock();

			Ogre::Vector3 wsChunkPos = chunk->getIndex().toVector3() * chunkSize;
			max = max + 1;
			return AxisAlignedBox(wsChunkPos + min.toVector3() * VT::Root::getSingleton().getInvVoxelDimMinusOne() * chunkSize, 
								  wsChunkPos + max.toVector3() * VT::Root::getSingleton().getInvVoxelDimMinusOne() * chunkSize);
		}

		return AxisAlignedBox();
	}


	//-----------------------------------------------------------------------
	Ogre::AxisAlignedBox OctreeChunkCreatorM2::getRegularBoundingBox(OctreeChunk* chunk, Ogre::Vector3& chunkSize)
	{
		Ogre::Vector3 wsChunkPos = chunk->getIndex().toVector3() * chunkSize;
		return AxisAlignedBox(wsChunkPos, wsChunkPos + chunkSize);
	}
	*/

	//-----------------------------------------------------------------------
	Ogre::AxisAlignedBox OctreeChunkCreatorM2::getTransitionBoundingBox(OctreeChunk* chunk, const Ogre::Vector3& chunkSize, Ogre::uint8 face, Ogre::uint8 side)
	{
		VT::Root* root = VT::Root::getSingletonPtr();

		Ogre::Vector3 wsChunkPos = chunk->getIndex().toVector3() * chunkSize;

		Ogre::Vector3 min = (OctreeChunk::msTransitionFace[face] * side) * chunkSize * root->getInvVoxelDimMinusOne() * (root->getVoxelDim()-2);

		Ogre::Vector3 max = min + OctreeChunk::msTransitionFace[face] * chunkSize * root->getInvVoxelDimMinusOne();
		max += OctreeChunk::msOneMinusTransitionFace[face] * chunkSize;

		return AxisAlignedBox(wsChunkPos + min, wsChunkPos + max);
	}

	//-----------------------------------------------------------------------
	void OctreeChunkCreatorM2::destroy(OctreeChunk* chunk, const bool clearCreationData)
	{
		if(!chunk->getParent() && chunk->rvs)
		{
			mSlicingToRecycle.push_back(chunk->rvs);
		}

		OctreeChunkCreator::destroy(chunk, clearCreationData); // deletes the chunk
	}



} // ends namespace VT
} // ends namespace GOOF
