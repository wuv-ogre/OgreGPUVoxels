#include "GOOFVTInfluenceVolume.h"
#include "GOOFVT.h"
#include "GOOFRenderTexture.h"
#include "GOOFNameGenerator.h"

namespace GOOF
{
    namespace VT
    {
        OctreeChunkManagerPlusInfluence::OctreeChunkManagerPlusInfluence(Ogre::SceneManager* sceneMgr, SharedParams* sharedParams, Ogre::Camera* camera, const Ogre::String& densityMaterialName, const Ogre::String& displayMaterialName, const Ogre::String& displayDeltaMaterialName, const Ogre::uint& influenceVolumeDim) :
            OctreeChunkManager(sceneMgr, sharedParams, camera, densityMaterialName, displayMaterialName, displayDeltaMaterialName),
            mInfluenceVolumeDim( influenceVolumeDim )
        {
        }
        
        RenderVolume* OctreeChunkManagerPlusInfluence::createInfluenceVolume()
        {
            return new RenderVolume(mCamera, GOOF::NameGenerator::Next("InfluenceRenderVolume"), Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, mInfluenceVolumeDim, mInfluenceVolumeDim, mInfluenceVolumeDim, Ogre::PF_FLOAT16_R);
        }
        
        OctreeChunk* OctreeChunkManagerPlusInfluence::createRoot( Ogre::SceneNode* sceneNode, RenderVolumeWithTimeStamp* rvs )
        {
            RootOctreeChunkPlusInfluence* root = new RootOctreeChunkPlusInfluence( );
            root->mSceneNode = sceneNode;
            root->mDensity = rvs;
            root->mInfluence = createInfluenceVolume();
            root->mDisplayMat = mDisplayMat->clone( GOOF::NameGenerator::Next("CloneDisplay") );
            root->mDisplayDeltaMat = mDisplayDeltaMat->clone( GOOF::NameGenerator::Next("ConeDisplayDelta") );

            root->mDisplayMat->getTechnique( "GBuffer" )->getPass(0)->getTextureUnitState(0)->setTexture( root->mInfluence->mTexture );
            root->mDisplayDeltaMat->getTechnique( "GBuffer" )->getPass(0)->getTextureUnitState(0)->setTexture( root->mInfluence->mTexture );

            return root;
        }
        
        InfluenceVolume::InfluenceVolume( RenderVolume* result,
                                          const Ogre::String& writeMaterialName,
                                          const Ogre::String& propagateMaterialName ) : mResult(result)
        {
            mWriteMat = Ogre::MaterialManager::getSingleton().load(writeMaterialName, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME).staticCast<Ogre::Material>();
            mPropagateMat = Ogre::MaterialManager::getSingleton().load(propagateMaterialName, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME).staticCast<Ogre::Material>();
            
            Ogre::GpuProgramParametersSharedPtr fparams = mPropagateMat->getTechnique(0)->getPass(0)->getFragmentProgramParameters();
            
            mMomentum = *fparams->getFloatPointer(fparams->getConstantDefinition("g_momentum").physicalIndex);
            mDecay = *fparams->getFloatPointer(fparams->getConstantDefinition("g_decay").physicalIndex);
            
            std::cout << "result " << result->mTexture->getName() << std::endl << std::endl;
        }
        
        Ogre::Vector3 InfluenceVolume::toTextureSpace( const Int3& index, const Ogre::Vector3& worldPos, const SharedParams* params )
        {
            Ogre::Vector3 worldSpaceToTextureSpaceScale = 1.0 / ( params->getLodChunkSize(0) + params->getLodVoxelSize(0) );
            
            Ogre::Vector3 min = index.toVector3() * params->getLodChunkSize(0);
            Ogre::Vector3 result = (worldPos - min) * worldSpaceToTextureSpaceScale;
            
            return result;
        }
        
        void InfluenceVolume::write( Ogre::SceneManager* sceneMgr, RenderVolume* rvs, const Int3& index, const std::vector<InfluencePoint>& points, const SharedParams* params )
        {
            for( const InfluencePoint& pt : points )
            {
                Ogre::Pass* pass = mWriteMat->getTechnique(0)->getPass(0);
                
                Ogre::Vector3 texPos = toTextureSpace( index, pt.worldPos, params );
                Ogre::Vector3 texScale( 1.0 / rvs->mTexture->getWidth() );
                
                pass->getFragmentProgramParameters()->setNamedConstant("g_influence", pt.value);
                
                
                rvs->renderSection(sceneMgr, pass, texPos, texScale);
            }
        }
        
        /*
        void InfluenceVolume::setPreserve( const bool preserve)
        {
            mPreserve = preserve;
            mPropagateMat->getTechnique(0)->getPass(0)->getFragmentProgramParameters()->setNamedConstant("g_preserve", mPreserve);
        }
        */
        
        void InfluenceVolume::setDecay( const Ogre::Real decay )
        {
            mDecay = decay;
            mPropagateMat->getTechnique(0)->getPass(0)->getFragmentProgramParameters()->setNamedConstant("g_decay", mDecay);
        }
        
        void InfluenceVolume::setMomentum( const Ogre::Real momentum )
        {
            mMomentum = momentum;
            mPropagateMat->getTechnique(0)->getPass(0)->getFragmentProgramParameters()->setNamedConstant("g_momentum", mMomentum);
        }

        void InfluenceVolume::propagate( Ogre::SceneManager* sceneMgr, RenderVolume*& rvs )
        {            
            Ogre::Pass* pass = mPropagateMat->getTechnique(0)->getPass(0);
            pass->getTextureUnitState(0)->setTexture( rvs->mTexture );
            mResult->render( sceneMgr, pass );
            std::swap( rvs, mResult );
        }
    }
}
