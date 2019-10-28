#include "GOOFVTDensity.h"
#include "GOOFVTRoot.h"
#include "GOOFSlicing.h"
#include "GOOFMaterialUtils.h"

using namespace Ogre;

namespace GOOF
{
namespace VT
{
	//-----------------------------------------------------------------------
	Density::Density()
	{		
		VT::Root* root = VT::Root::getSingletonPtr();

		String materialName = "VT/Density";
		
		String vertProgramName = materialName + "_VPCG";
		String fragProgramName = materialName + "_FPCG";
		
		HighLevelGpuProgramPtr hgp = 
		HighLevelGpuProgramManager::getSingleton().createProgram(vertProgramName,
																 ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
																 "cg",
																 GPT_VERTEX_PROGRAM);
		hgp->setSourceFile("RVDensity_vp.cg.h");
		hgp->setParameter("profiles", "vs_3_0 vp40 glslv");
		hgp->setParameter("entry_point", "main");
		hgp->load();
		
		hgp = 
		HighLevelGpuProgramManager::getSingleton().createProgram(fragProgramName,
																 ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
																 "cg",
																 GPT_FRAGMENT_PROGRAM);
		hgp->setSourceFile("RVDensity_fp.cg.h");
		hgp->setParameter("profiles", "ps_3_0 fp40 glslf");
		hgp->setParameter("entry_point", "main");
		hgp->load();
		
		mMaterial = MaterialManager::getSingleton().create(materialName,
														   ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
		
		Pass* pass = mMaterial->getTechnique(0)->getPass(0);
		pass->setDepthCheckEnabled(false);
		pass->setDepthWriteEnabled(false);
		pass->setVertexProgram(vertProgramName);
		pass->getVertexProgramParameters()->addSharedParameters( root->getGpuSharedParameter() );
		pass->setFragmentProgram(fragProgramName);
		pass->getFragmentProgramParameters()->addSharedParameters( root->getGpuSharedParameter() );
		
		// Match named params to AutoConstantDefinition names and set where matching
		MaterialUtils::SetConstants(pass);
		
        /*
		// All the noise volumes
		root->CreateTextureUnitStates(pass,
									  VT::Root::TT_PERLIN_NOISE |
									  //VT::Root::TT_PACKED_PERLIN_NOISE |
									  VT::Root::TT_RANDOM_NOISE);
        */

		// "heightmap_west_us.png"
		TextureUnitState* tunit = pass->createTextureUnitState();
		

		//"heightmap_global_blur.png"
		//"heightmap.png"
		//"heightmap_west_us.png"

		tunit->setTextureName("heightmap.png", TEX_TYPE_2D);
		tunit->setTextureAddressingMode(TextureUnitState::TAM_WRAP);

		//MaterialSerializer serializer;
		//serializer.queueForExport(mat);
		//const Ogre::String materalContents = serializer.getQueuedAsString();
		//Ogre::LogManager::getSingletonPtr()->logMessage(materalContents);





        /*
		mCompositorName = mMaterial->getName() + "Slice";
		CompositorPtr compositor = CompositorManager::getSingleton().create(mCompositorName,
																			ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
		
		CompositionTargetPass* targetPass = compositor->createTechnique()->getOutputTargetPass();
		targetPass->setInputMode(CompositionTargetPass::IM_NONE);
		CompositionPass* compositorPass;
		compositorPass = targetPass->createPass();
		compositorPass->setType(CompositionPass::PT_CLEAR);
		compositorPass = targetPass->createPass();
		compositorPass->setType(CompositionPass::PT_RENDERQUAD);
		compositorPass->setMaterialName(mMaterial->getName());
        */
	}

	//-----------------------------------------------------------------------
	Density::~Density()
	{
		//CompositorManager::getSingleton().remove(mCompositorName);

		Ogre::uint numPasses = mMaterial->getTechnique(0)->getNumPasses();
		for(int i=0; i< numPasses; i++)
		{
			Pass* pass = mMaterial->getTechnique(0)->getPass(i);
			GpuProgramPtr prog = pass->getVertexProgram();
			HighLevelGpuProgramManager::getSingleton().remove(prog->getHandle());
			prog = pass->getFragmentProgram();
			HighLevelGpuProgramManager::getSingleton().remove(prog->getHandle());
		}
		
		MaterialManager::getSingleton().remove(mMaterial->getHandle());
	}
	
	//-----------------------------------------------------------------------
	void Density::render(Ogre::SceneManager* sceneMgr, Slicing* target)
	{
		GpuProgramParametersSharedPtr vparams = mMaterial->getTechnique(0)->getPass(0)->getVertexProgramParameters();
		target->render(sceneMgr, mMaterial, vparams);

		//target->render(mCompositorName, vparams);
	}
}
}
