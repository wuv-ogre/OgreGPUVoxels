#include "GOOFVTApplication2.h"
#include "GOOFInterfaceOIS.h"
#include "GOOFSharedData.h"
#include "GOOFDebugTextureManager.h"
#include "GOOFFreeCamera.h"
#include "GOOFGpuProgramUtil.h"
#include "GOOFResourcesRecursive.h"
#include "GOOFProceduralRenderable.h"
#include "GOOFVT.h"
#include "GOOFVTNibbler.h"
#include "GOOFVTOctreeChunk.h"
#include "GOOFVTInfluenceVolume.h"
#include "GOOFRenderTexture.h"
#include "GOOFNameGenerator.h"
#include "GOOFStopwatch.h"
#include "DSSystem.h"
#include "DSAtmosphereScattering.h"
#include "CraftscapeTerrain.h"
#include "CEGUIHelpers.h"
#include "CEGUIComboWidgets.h"
#include <CEGUI/CEGUI.h>
#include <OgreWireBoundingBox.h>

using namespace Ogre;

namespace GOOF
{
    VTApplication2::VTApplication2() :
    ApplicationHierarchy(),
    mOctreeChunkManager(0),
    mOctreeChunkManagerSpherePolicy(0),
    mInfluenceVolume(0),
    mRaycast(0),
    mNibbler(0),
    mLodStratedy(0),
    mOctreeChunkSelector(0),
    mDebugNode(0),
    mAdjustLodsToggleButton(0),
    mPauseLightAnimation(true),
    mDenistyMatName("VT/Density"),
    mDisplayMatName("VT/Display"),
    mDisplayDeltaMatName("VT/DisplayDelta")
    {
    }
    
    VTApplication2::~VTApplication2()
    {
        if(mOctreeChunkSelector)
            delete mOctreeChunkSelector;
        if(mOctreeChunkManager)
            delete mOctreeChunkManager;
        if(mRaycast)
            delete mRaycast;
        if(mNibbler)
            delete mNibbler;
        if(mOctreeChunkManagerSpherePolicy)
            delete mOctreeChunkManagerSpherePolicy;
    }
    
    void VTApplication2::addResourceLocations()
    {
        ApplicationHierarchy::addResourceLocations();
        
    }
    void VTApplication2::testGeometryShaders()
    {
        const String GLSL_MATERIAL_NAME = "Ogre/GPTest/SwizzleGLSL";
        const String ASM_MATERIAL_NAME = "Ogre/GPTest/SwizzleASM";
        const String CG_MATERIAL_NAME = "Ogre/GPTest/SwizzleCG";
        
        //Check capabilities
        const RenderSystemCapabilities* caps = Root::getSingleton().getRenderSystem()->getCapabilities();
        if (!caps->hasCapability(RSC_GEOMETRY_PROGRAM))
        {
            OGRE_EXCEPT(Ogre::Exception::ERR_NOT_IMPLEMENTED, "Your card does not support geometry programs, so cannot "
                        "run this demo. Sorry!",
                        "GeometryShading::createScene");
        }
        
        int maxOutputVertices = caps->getGeometryProgramNumOutputVertices();
        Ogre::LogManager::getSingleton().getDefaultLog()->stream() <<
        "Num output vertices per geometry shader run : " << maxOutputVertices;
        
        Entity *ent = mSceneMgr->createEntity("head", "ogrehead.mesh");
        
        String materialName = GLSL_MATERIAL_NAME;
        //String materialName = ASM_MATERIAL_NAME;
        //String materialName = CG_MATERIAL_NAME;
        
        //Set all of the material's sub entities to use the new material
        for (unsigned int i=0; i<ent->getNumSubEntities(); i++)
        {
            ent->getSubEntity(i)->setMaterialName(materialName);
        }
        
        //Add entity to the root scene node
        mSceneMgr->getRootSceneNode()->createChildSceneNode()->attachObject(ent);
        
        mViewport->setBackgroundColour(ColourValue::Green);
    }
    
    void VTApplication2::testRegular(VT::SharedParams* sharedParams)
    {
        Int3 idx(0, -1, 0); // chunk has something in it?
        sharedParams->setRegularParams(idx, 0);
        
        //params.logParams();
        
        Ogre::uint rvsSize = sharedParams->getRenderVolumeSize();
        RenderVolume* denisty = new RenderVolume(mCamera, "rvs", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, rvsSize, rvsSize, rvsSize, Ogre::PF_FLOAT16_R);
        
        Ogre::MaterialPtr densityMat = Ogre::MaterialManager::getSingleton().load(mDenistyMatName, ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME).staticCast<Ogre::Material>();
        denisty->render(mSceneMgr, densityMat->getTechnique(0)->getPass(0));
        
        GOOF::DebugTextureManager::getSingleton().addTexture(denisty->mTexture, Ogre::ColourValue(1,0,0,0));
        
        VT::RegularSeed* regularSeed = new VT::RegularSeed(mSceneMgr, sharedParams);
        VT::ListTriangles* listTriangles = new VT::ListTriangles(mSceneMgr, regularSeed->mSeedObject, sharedParams);
        listTriangles->setDensity(denisty->mTexture);
        listTriangles->update(mSceneMgr);
        listTriangles->log();
        
        
        VT::GenTriangles* genTriangles = new VT::GenTriangles( VT::RegularTriTableTexture().mTriTable );
        ProceduralRenderable* object = static_cast<ProceduralRenderable*>(mSceneMgr->createMovableObject(ProceduralRenderableFactory::FACTORY_TYPE_NAME));
        genTriangles->setDensity(denisty->mTexture);
        genTriangles->update(mSceneMgr, listTriangles->mList, object);
        genTriangles->log(object, 10);
        
        object->setMaterialName(mDisplayMatName);
        object->setBoundingBox(Ogre::AxisAlignedBox::BOX_INFINITE);
        mSceneMgr->getRootSceneNode()->attachObject(object);
    }
    
    void VTApplication2::testRegularBatch(VT::SharedParams* sharedParams)
    {
        Ogre::uint rvsSize = sharedParams->getRenderVolumeSize();
        RenderVolume* denisty = new RenderVolume(mCamera, "rvs", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, rvsSize, rvsSize, rvsSize, Ogre::PF_FLOAT16_R);
        Ogre::MaterialPtr densityMat = Ogre::MaterialManager::getSingleton().load(mDenistyMatName, ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME).staticCast<Ogre::Material>();
        
        VT::RegularSeed* regularSeed = new VT::RegularSeed(mSceneMgr, sharedParams);
        VT::ListTriangles* listTriangles = new VT::ListTriangles(mSceneMgr, regularSeed->mSeedObject, sharedParams);
        listTriangles->setDensity( denisty->mTexture );
        
        VT::GenTriangles* genTriangles = new VT::GenTriangles( VT::RegularTriTableTexture().mTriTable );
        genTriangles->setDensity( denisty->mTexture );
        
        VT::GenTrianglesDelta* genTrianglesDelta = new VT::GenTrianglesDelta( VT::RegularTriTableTexture().mTriTable );
        genTrianglesDelta->setDensity( denisty->mTexture );
        
        Int3 min(-4,-1,-4);
        Int3 max(3,1,3);
        Int3 idx;
        for(idx.x = min.x; idx.x <= max.x; idx.x++)
        {
            for(idx.y = min.y; idx.y <= max.y; idx.y++)
            {
                for(idx.z = min.z; idx.z <= max.z; idx.z++)
                {
                    sharedParams->setRegularParams(idx, 0);
                    
                    denisty->render(mSceneMgr, densityMat->getTechnique(0)->getPass(0));
                    
                    listTriangles->update(mSceneMgr);
                    //listTriangles->log();
                    
                    ProceduralRenderable* mObject = static_cast<ProceduralRenderable*>(mSceneMgr->createMovableObject(ProceduralRenderableFactory::FACTORY_TYPE_NAME));
                    genTrianglesDelta->update(mSceneMgr, listTriangles->mList, mObject);
                    //genTrianglesDelta->log(mObject, 10);
                    
                    mObject->setMaterialName(mDisplayDeltaMatName);
                    mObject->setBoundingBox(Ogre::AxisAlignedBox::BOX_INFINITE);
                    mSceneMgr->getRootSceneNode()->attachObject(mObject);
                }
            }
        }
    }
    
    
    void VTApplication2::testTransition(VT::SharedParams* sharedParams)
    {
        //params.logParams();
        
        Ogre::uint rvsSize = sharedParams->getRenderVolumeSize();
        RenderVolume* denisty = new RenderVolume(mCamera, "rvs", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, rvsSize, rvsSize, rvsSize, Ogre::PF_FLOAT16_R);
        Ogre::MaterialPtr densityMat = Ogre::MaterialManager::getSingleton().load(mDenistyMatName, ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME).staticCast<Ogre::Material>();
        
        VT::RegularSeed* regularSeed = new VT::RegularSeed(mSceneMgr, sharedParams);
        VT::ListTriangles* listTriangles = new VT::ListTriangles(mSceneMgr, regularSeed->mSeedObject, sharedParams);
        listTriangles->setDensity( denisty->mTexture );
        
        VT::GenTriangles* genTriangles = new VT::GenTriangles( VT::RegularTriTableTexture().mTriTable );
        genTriangles->setDensity( denisty->mTexture );
        
        VT::GenTrianglesDelta* genTrianglesDelta = new VT::GenTrianglesDelta( VT::RegularTriTableTexture().mTriTable );
        genTrianglesDelta->setDensity( denisty->mTexture );
        
        VT::TransitionSeed* transitionSeed = new VT::TransitionSeed(mSceneMgr, sharedParams);
        VT::TransitionListTriangles* transitionListTriangles = new VT::TransitionListTriangles(mSceneMgr, transitionSeed->mSeedObject, sharedParams);
        transitionListTriangles->setDensity( denisty->mTexture );
        
        VT::TransitionGenTriangles* transitionGenTriangles = new VT::TransitionGenTriangles( VT::TransitionTriTableTexture().mTriTable );
        transitionGenTriangles->setDensity( denisty->mTexture );
        
        std::vector<Int3> list;
        list.push_back(Int3(0,0,0));
        list.push_back(Int3(0,0,1));
        list.push_back(Int3(1,0,1));
        list.push_back(Int3(1,0,0));
        
        for(std::vector<Int3>::iterator it = list.begin(); it != list.end(); ++it)
        {
            sharedParams->setRegularParams(*it, 0);
            
            denisty->render(mSceneMgr, densityMat->getTechnique(0)->getPass(0));
            
            listTriangles->update(mSceneMgr);
            //listTriangles->log();
            
            ProceduralRenderable* mObject = static_cast<ProceduralRenderable*>(mSceneMgr->createMovableObject(ProceduralRenderableFactory::FACTORY_TYPE_NAME));
            genTrianglesDelta->update(mSceneMgr, listTriangles->mList, mObject);
            //genTrianglesDelta->log(mObject, 10);
            
            mObject->setMaterialName(mDisplayDeltaMatName);
            mObject->setBoundingBox(Ogre::AxisAlignedBox::BOX_INFINITE);
            mSceneMgr->getRootSceneNode()->attachObject(mObject);
            
            for(int side=0; side<2; side++)
            {
                Ogre::Vector4 deltaFaces(0,0,0,0);
                
                for(int face=0; face<3; face++)
                {
                    deltaFaces[face] = 1;
                    
                    sharedParams->setTransitionParams(face, side);
                    
                    transitionListTriangles->update(mSceneMgr);
                    transitionListTriangles->log(100);
                    
                    ProceduralRenderable* mObject = static_cast<ProceduralRenderable*>(mSceneMgr->createMovableObject(ProceduralRenderableFactory::FACTORY_TYPE_NAME));
                    transitionGenTriangles->update(mSceneMgr, transitionListTriangles->mList, mObject);
                    
                    mObject->setMaterialName(mDisplayMatName);
                    mObject->setBoundingBox(Ogre::AxisAlignedBox::BOX_INFINITE);
                    mSceneMgr->getRootSceneNode()->attachObject(mObject);
                }
                mObject->setCustomParameter(side, deltaFaces);
                //mObject->setVisible(false);
            }
        }
    }
    
    void VTApplication2::testDensityArrayVsVolumeSpeed(VT::SharedParams* sharedParams)
    {
        sharedParams->setRegularParams(Int3(0,0,0), 0);
        Ogre::uint rvsSize = sharedParams->getRenderVolumeSize();
        
        GOOF::RenderTexture* array = new GOOF::RenderTexture(mCamera, "Array", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, rvsSize*rvsSize, rvsSize, Ogre::PF_FLOAT16_R);
        Ogre::MaterialPtr mat = Ogre::MaterialManager::getSingleton().load("VT/DensityArray", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME).staticCast<Material>();
        array->render(mSceneMgr, mat->getTechnique(0)->getPass(0)); // first call params not getting set, future calls they are..
        
        Stopwatch stopwatch;
        for(int i=0; i<100; i++)
        {
            stopwatch.start();
            array->render(mSceneMgr, mat->getTechnique(0)->getPass(0));
            stopwatch.stop();
        }
        stopwatch.log( "VT/DensityArray" );
        
        stopwatch.reset();
        GOOF::RenderVolume* vol = new GOOF::RenderVolume(mCamera, "Volume", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, rvsSize, rvsSize, rvsSize, Ogre::PF_FLOAT16_R);
        mat = Ogre::MaterialManager::getSingleton().load("VT/Density", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME).staticCast<Material>();
        for(int i=0; i<100; i++)
        {
            stopwatch.start();
            vol->render(mSceneMgr, mat->getTechnique(0)->getPass(0));
            stopwatch.stop();
        }
        stopwatch.log( "VT/Density     " );
        
        
        GOOF::DebugTextureManager::getSingleton().addTexture(array->mTexture);
        GOOF::DebugTextureManager::getSingleton().addTexture(vol->mTexture);
    }
    
    
    
    
    void VTApplication2::testRenderVolumeSubsectionRender(VT::SharedParams* sharedParams)
    {
        sharedParams->setRegularParams(Int3(0,0,0), 0);
        Ogre::uint rvsSize = sharedParams->getRenderVolumeSize();
        
        GOOF::RenderVolume* rvs = new GOOF::RenderVolume(mCamera, "Volume", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, rvsSize, rvsSize, rvsSize, Ogre::PF_FLOAT16_RGB);
        
        Ogre::MaterialPtr mat = Ogre::MaterialManager::getSingleton().load("VT/Nibbler", Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME).staticCast<Ogre::Material>();
        Ogre::Pass* pass = mat->getTechnique(0)->getPass(0);
        
        rvs->renderSection(mSceneMgr, pass, Vector3(0.5, 0.5, 0.5), Vector3(0.08, 0.08, 0.08));
        //rvs->renderSection(mSceneMgr, pass, Vector3(0.5, 0.5, 0.5), Vector3(1.0, 1.0, 1.0));
        
        GOOF::DebugTextureManager::getSingleton().addTexture(rvs->mTexture);
    }
    
    void VTApplication2::testRenderObjectToVolume()
    {
        /**
         * Method
         * For each rv slice
         *   move the camera so its forward 1 slice and render mesh with front faces to a 2D render texture and store depth
         *   we render the mesh with back faces in the rv
         *   do a depth comparision between the forward slice and the slice
         */
        
        RenderVolume* front = new RenderVolume(mCamera, GOOF::NameGenerator::Next("RenderVolume"), Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, 64, 64, 64, Ogre::PF_FLOAT16_RGBA);
        front->clear(Ogre::FBT_COLOUR | Ogre::FBT_DEPTH, Ogre::ColourValue::Black ); // clear the depth
        
        
        MeshPtr mesh = Ogre::MeshManager::getSingleton().load("Sphere1m.mesh", ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME);
        
        // PassThrough/Depth Debug/Numbers
        Ogre::MaterialPtr mat = MaterialManager::getSingleton().load("PassThrough/Depth", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME).staticCast<Material>();
        Ogre::Pass* pass = mat->getTechnique(0)->getPass(0);
        
        float dim = 10.0;
        
        
        Ogre::Camera cam("internal", mSceneMgr);
        cam.setProjectionType(PT_ORTHOGRAPHIC);
        cam.setOrthoWindow(dim, dim);
        cam.setNearClipDistance(0.001);
        cam.setFarClipDistance(dim);
        
        Ogre::RenderOperation ro;
        mesh->getSubMesh(0)->_getRenderOperation(ro);
        
        Ogre::TexturePtr mTexture = front->mTexture;
        
        int depth = mTexture->getDepth();
        
        for(Ogre::uint i=0; i<depth; i++)
        {
            float z = (float)i/depth*dim - dim/2 - cam.getNearClipDistance();
            
            cam.setPosition(Vector3(0, 0, z)); // (float)i/depth*dim - dim/2)
            Matrix4 viewMatrix = cam.getViewMatrix();
            
            //PrintOgre::Matrix4(viewMatrix);
            //Matrix4 viewMatrix;
            //cam.calcViewMatrixRelative(Vector3(0, 0, z), viewMatrix);
            
            Ogre::Matrix4 trans;
            trans.makeTransform(Ogre::Vector3(2.0, 2.0, 2.0), Ogre::Vector3(2.0, 2.0, 2.0), Ogre::Quaternion::IDENTITY);
            
            Matrix4 projMatrix = cam.getProjectionMatrixWithRSDepth();
            
            
            Ogre::RenderTarget* rt = mTexture->getBuffer()->getRenderTarget(i);
            
            mSceneMgr->manualRender(&ro,
                                    pass,
                                    rt->getViewport(0),
                                    trans,                   // << set trans here, world would be identity in the camera
                                    viewMatrix,
                                    projMatrix,
                                    false);
        }
        
        mViewport->setBackgroundColour(ColourValue(0.3, 0.3, 0.3, 1.0));
        GOOF::DebugTextureManager::getSingleton().addTexture(front->mTexture);
    }
    
    void VTApplication2::testLodPointGeneration(VT::SharedParams* sharedParams)
    {
        int PixelCoverage = 5;
        
        /*
         Ogre::Vector3 voxelSize = sharedParams->getLodVoxelSize(0);
         Ogre::Real avDim = (voxelSize.x + voxelSize.y + voxelSize.z)/3.0;
         Real outleft, outright, outtop, outbottom;
         mCamera->getFrustumExtents(outleft, outright, outtop, outbottom);
         Ogre::Real avDimSq = avDim * avDim;
         Ogre::Real z = avDimSq/(outright * outtop);
         */
        
        for(int i=0; i<sharedParams->getLodCount(); i++)
        {
            Ogre::Vector3 voxelSize = sharedParams->getLodVoxelSize(i);
            Ogre::Real avDim = (voxelSize.x + voxelSize.y + voxelSize.z)/3.0;
            
            //for each z unit we go one outright outtop per pixel
            
            
            //mCamera->getFOVy();
            Real outleft, outright, outtop, outbottom;
            mCamera->getFrustumExtents(outleft, outright, outtop, outbottom);
            printf("outleft %f, outright %f, outtop %f, outbottom %f\n", outleft, outright, outtop, outbottom);
            
            // to maintain 4 pixels, the change in x and y per unit z
            //outright = outright * 2 * PixelCoverage / (float) mViewport->getActualWidth();
            //outtop = outtop * 2 * PixelCoverage / (float) mViewport->getActualHeight();
            
            printf("%d %d\n", mViewport->getActualWidth(), mViewport->getActualHeight());
            
            Ogre::Real avDimSq = avDim * avDim;
            
            printf("for PixelCoverage %d avDimSq %f outright %f outtop %f\n", PixelCoverage, avDimSq, outright, outtop);
            
            
            //z*outright + z*outtop = avDimSq
            // avDimSq/z = outright + outtop
            // z = avDimSq/(outright + outtop)
            
            Ogre::Real z = avDimSq/(outright * outtop);
            
            printf("lod %d z %f\n\n", i, z);
            
            /*
             (objRadius * objRadius * PI * viewport->getActualWidth() * viewport->getActualHeight() * projectionMatrix[0][0] * projectionMatrix[1][1] * bias) / distanceSquared
             */
        }
    }
    
    void VTApplication2::testInfluenceVolume(VT::SharedParams* sharedParams)
    {
        Int3 index(0, 0, 0); // imaginary chunks indexs
        sharedParams->setRegularParams(index, 0);
        
        RenderVolume* rvs = new RenderVolume(mCamera, GOOF::NameGenerator::Next("RenderVolume"), Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, 64, 64, 64, Ogre::PF_FLOAT16_R);
        
        Ogre::Vector3 chunkSize = sharedParams->getLodChunkSize(0);
        
        std::vector< VT::InfluencePoint > points;
        for(int i=0; i<20; i++)
        {
            VT::InfluencePoint pt;
            pt.worldPos = index.toVector3() * chunkSize + Ogre::Vector3( Ogre::Math::UnitRandom(), Ogre::Math::UnitRandom(), Ogre::Math::UnitRandom() ) * chunkSize;
            pt.value = Ogre::Math::UnitRandom() * 2.0 - 1.0;
            points.push_back(pt);
        }
        
        RenderVolume* result = new RenderVolume(mCamera, GOOF::NameGenerator::Next("RenderVolume"), Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, 64, 64, 64, Ogre::PF_FLOAT16_R);
        
        VT::InfluenceVolume iv(result);
        iv.write( mSceneMgr, rvs, index, points, sharedParams );
        
        for(int i=0; i<50; i++)
        {
            iv.propagate( mSceneMgr, rvs );
        }
        
        GOOF::DebugTextureManager::getSingleton().addTexture( rvs->mTexture );
        
        for(int i=-1; i<=1; i++)
        {
            for(int j=-1; j<=1; j++)
            {
                for(int k=-1; k<=1; k++)
                {
                    std::cout << "( " << i << ", " << j << ", " << k << " )" << std::endl;
                }
            }
        }
    }
    
    void VTApplication2::createOctreeChunks(const Int3& min, const Int3& max)
    {
        assert( mOctreeChunkManager );
        
        bool recurse = true;
        
        // grid of chunks
        Int3 idx;
        for(idx.x = min.x; idx.x <= max.x; idx.x++)
        {
            for(idx.y = min.y; idx.y <= max.y; idx.y++)
            {
                for(idx.z = min.z; idx.z <= max.z; idx.z++)
                {
                    mOctreeChunkManager->create(idx, 0, 0, recurse);
                }
            }
        }
        
        Ogre::String log;
        mOctreeChunkManager->logStopwatches(log);
        Ogre::LogManager::getSingleton().logMessage(log);
        
        VT::OctreeChunkManager::ChunkIterator it = mOctreeChunkManager->getChunkIterator();
        while(it.hasMoreElements())
        {
            VT::OctreeChunk* chunk = it.getNext();
            setChunkVisible(chunk, mVisibleLodLevel);
            //GOOF::DebugTextureManager::getSingleton().addTexture(chunk->rvs->mTexture);
        }
    }
    
    VT::OctreeChunk* VTApplication2::findNearest( const Ogre::Ray& ray, Ogre::Real& dist )
    {
        dist = Ogre::Math::POS_INFINITY;
        
        mQuery.getSceneQuery()->setRay(ray);
        mQuery.getSceneQuery()->setSortByDistance(true);
        mQuery.execute();
        
        for(VT::RaySceneQueryOctreeChunk::QueryResultVector::iterator it = mQuery.getResults().begin(); it != mQuery.getResults().end(); ++it)
        {
            VT::OctreeChunk* chunk = (*it).object;
            if(chunk->mRenderable->isVisible())
            {
                mRaycast->update(mSceneMgr, chunk->mRenderable, ray, dist);
                
                if(dist != Ogre::Math::POS_INFINITY)
                {
                    mQuery.clear();
                    return chunk;
                }
            }
        }
        return 0;
    }
    
    void VTApplication2::changeDensityToCraftscape()
    {
        Craftscape::Terrain terrain(512, 512);
        terrain.loadFromFileSet("");
        
        Ogre::MaterialPtr mat = Ogre::MaterialManager::getSingleton().load("VT/Density/Craftscape", Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME).staticCast<Ogre::Material>();
        Ogre::Pass* pass = mat->getTechnique(0)->getPass(0);
        pass->getTextureUnitState(0)->setTexture(terrain.mHeights->mTexture);
        
        mDenistyMatName = mat->getName();
    }
    
    void VTApplication2::setupShadows()
    {
        mCamera->setFarClipDistance(5000.0);
        mCamera->setNearClipDistance(mCamera->getFarClipDistance()/1000.0);
        
        // without this pssm doesn't work correctly
        mSceneMgr->setShadowFarDistance( mCamera->getFarClipDistance() * 1.0 );
        
        ApplicationHierarchy::setupShadows();
    }
    
    


    
    
    bool VTApplication2::setupFinish()
    {
        bool success = ApplicationHierarchy::setupFinish();
        
        SharedData::getSingleton().mDSSystem->setEnabled(true);
        
        
        if(SharedData::getSingleton().mDSSystem->getEnabled())
        {
            mDisplayMatName = "VT/DS/Display";
            mDisplayDeltaMatName = "VT/DS/DisplayDelta";
        }
        
        // A definitive list of the syntaxes supported by the current card
        GpuProgramManager::getSingleton().getSupportedSyntax();
        
        // so after camera updated but before and draw calls
        connectUpdateEvent(UE_STARTED, boost::signals2::at_back);
        connectKeyEvents(KE_PRESSED);
        
        mEditor->getEditorWindow()->setText("Voxel Terrain");
        
        mSceneMgr->setAmbientLight(ColourValue(0.25, 0.25, 0.25)); // 0.33
        
        // Create Camera system
        mSceneMgr->getRootSceneNode()->createChildSceneNode()->attachObject(mCamera);
        mCamera->setFixedYawAxis(true);
        mCamera->getParentSceneNode()->setPosition(-2, 56, 81);
        
        
        FreeCamera* cameraSys = new FreeCamera(mCamera);
        cameraSys->setEnabled(true);
        cameraSys->setMovementRate(1500);
        SharedData::getSingleton().mCameraSystemNames.push_back(typeid(FreeCamera).name());
        SharedData::getSingleton().mUserObjectBindings.setUserAny(typeid(FreeCamera).name(), Ogre::Any( static_cast<CameraSystem*>(cameraSys) ));
        
        
        Ogre::Light* light;
        Ogre::Vector3 dir;
        light = mSceneMgr->createLight();
        light->setType(Ogre::Light::LT_DIRECTIONAL);
        dir = Ogre::Vector3(0.3, -1, 0.2);
        dir.normalise();
        light->setDirection(Ogre::Vector3::UNIT_Z);
        light->setDiffuseColour(Ogre::ColourValue(0.77, 0.75, 0.62, 1));
        light->setSpecularColour(Ogre::ColourValue(0.77, 0.75, 0.62, 1));
        light->setPosition(3, 70, 1);
        if(mSceneMgr->getShadowTextureCountPerLightType(Ogre::Light::LT_DIRECTIONAL) > 1)
        {
            light->setCustomShadowCameraSetup(SharedDataAccessor::getShadowCameraSetup("PSSM"));
        }
        else
        {
            light->setCustomShadowCameraSetup(SharedDataAccessor::getShadowCameraSetup("LiSPSM"));
        }
        light->setCastShadows(true);
        mDirLightNode = mSceneMgr->getRootSceneNode()->createChildSceneNode();
        mDirLightNode->attachObject(light);
        mDirLightNode->setOrientation(Ogre::Vector3::UNIT_Z.getRotationTo(dir));
        
        
        // see GOOF::Math GetFogParams for other fog types but exp2 is most realistic
        Ogre::Real d = mCamera->getFarClipDistance();
        Ogre::Real expDensity = Ogre::Math::Sqrt( Ogre::Math::Log( 1.0 / 0.0019) )/d; // 1.54x10^-4
        mSceneMgr->setFog(Ogre::FOG_EXP2, Ogre::ColourValue::White, expDensity, 0, 0);
        
        
        bool enabled = true;
        bool isPlanarWorld = true;
        // needs a very large radius otherwise the plane to sphere conversion will cause artifacts in shader
        DS::AtmosphereScattering::SetLightParams(light, enabled, isPlanarWorld, 1000000.0);
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        mDebugNode = mSceneMgr->getRootSceneNode()->createChildSceneNode();
        Ogre::Entity* ent = mSceneMgr->createEntity("Sphere0Point5m.mesh" );
        ent->setMaterialName("TestWhite");
        mDebugNode->attachObject(ent);
        mDebugNode->setScale(Ogre::Vector3::UNIT_SCALE * 20);
        
        mQuery.setSceneQuery(mSceneMgr->createRayQuery(Ogre::Ray()));
        
        Ogre::Vector3 chunkSizeLodZero(400, 400, 400);
        Ogre::uint8 lodCount = 3;
        Ogre::uint8 voxelDim = 16; // do not change
        VT::SharedParams* params = new VT::SharedParams(chunkSizeLodZero, lodCount, voxelDim);
        
        changeDensityToCraftscape(); // use VT_Density_Crafscape_fp.glsl instead of default VT_Density_fp.glsl
        
        mRoot->addMovableObjectFactory( OGRE_NEW GOOF::ProceduralRenderableFactory() );
        
        //Ogre::String log;
        //params->logParams(log);
        //Ogre::LogManager::getSingleton().logMessage(log);
        //testRegular(params);
        //testInfluenceVolume(params);
        //testRegularBatch(params);
        //testTransition(params);
        //testLodPointGeneration(params);
        //testOctreeChunkManager(params, Int3(-3,-1,-3), Int3(3,1,3) ); // creates a grid of chunks
        //testDensityArrayVsVolumeSpeed(params);
        //return true;
        
        bool plusInfluence = true;
        if(!plusInfluence)
        {
            mOctreeChunkManager = new VT::OctreeChunkManager(mSceneMgr, params, mCamera, mDenistyMatName, mDisplayMatName, mDisplayDeltaMatName);
        }
        else
        {
            VT::OctreeChunkManagerPlusInfluence* mgr = new VT::OctreeChunkManagerPlusInfluence(mSceneMgr, params, mCamera, mDenistyMatName, mDisplayMatName, mDisplayDeltaMatName);
            mOctreeChunkManager = mgr;
            mInfluenceVolume = new VT::InfluenceVolume( mgr->createInfluenceVolume() );
        }
        createOctreeChunks(  Int3(-3,-1,-3), Int3(3,1,3) );
        
        // Single ray at polygon level
        mRaycast = new VT::RayCast(mSceneMgr, params);
        
        // Nibbles density volume
        mNibbler = new VT::Nibbler(params);
        
        mOctreeChunkManagerSpherePolicy = new VT::OctreeChunkManagerSpherePolicy(params, false);
        
        // Selection of chunks (for debugging) show aab, display info. etc
        mOctreeChunkSelector = new VT::OctreeChunkSelector();
        
        // Uses PSSM like split points to change lod level of chunks see VT::SimpleLodStratedy::calculateSplitPoints operates on a ChunkManager
        mLodStratedy = new VT::SimpleLodStratedy(params);
        //mLodStratedy = new VT::SmartLodStratedy(params);
        mLodStratedy->calculateSplitPoints(mCamera->getNearClipDistance(), 500.0*4, 0.9f);
        
        
        
        SharedData::getSingleton().mDSSystem->setMode(DS::System::DSM_ALBEDO);

        
        
        
        
        
        CEGUI::Style style;
        style.frameEnabled = true;
        style.backgroundEnabled = true;
        
        CEGUI::Window* tc = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow()->getChildRecursive("Editor/TabControl");
        
        // VT tab
        {
            if(mOctreeChunkManager)
            {
                CEGUI::VerticalLayoutContainer* layout = CEGUI::Helpers::CreateOrRetrieveTabAndLayout(style.scheme, tc, "VT", false);
                
                auto create_mcl_item = [](CEGUI::MultiColumnList* mcl, Ogre::String name, uint col_id, uint row_idx)
                {
                    static const Ogre::String selectionBrush = "OgreTrayImages/GenericBrush";
                    CEGUI::ListboxTextItem* item = new CEGUI::ListboxTextItem( name );
                    item->setSelectionBrushImage(selectionBrush);
                    item->setAutoDeleted( true );
                    mcl->setItem(item, col_id, row_idx);
                };
                
                // octree chunk manager stopwatches
                {
                    CEGUI::MultiColumnList* mcl = static_cast<CEGUI::MultiColumnList*>(layout->createChild(style.scheme + "/MultiColumnList", "VT/Info/MultiColumn"));
                    CEGUI::UDim w = cegui_reldim( 0.99/7.0 );
                    
                    int col_idx = 0;
                    
                    std::vector< std::string > columnNames =
                    {
                        "item", "%", "av", "count", "accum", "best", "worst"
                    };
                    for( const std::string& name : columnNames )
                    {
                        mcl->addColumn(name,  col_idx++, w);
                    }
                    
                    mcl->setWidth( cegui_reldim(1.0f) );
                    mcl->setShowHorzScrollbar( false );
                    mcl->setShowVertScrollbar( false );
                    mcl->setUserColumnDraggingEnabled( false );
                    mcl->setUserColumnSizingEnabled( true );
                    
                    Ogre::uint64 totalAccum = 0;
                    for( const Stopwatch* stopwatch : mOctreeChunkManager->getStopwatchs() )
                    {
                        totalAccum += stopwatch->mAccum;
                    }
                    
                    for( const Stopwatch* stopwatch : mOctreeChunkManager->getStopwatchs() )
                    {
                        uint row_idx = mcl->addRow( mcl->getRowCount() + 1 ) ;
                        
                        col_idx = 0;
                        create_mcl_item( mcl, stopwatch->name, col_idx++, row_idx );
                        create_mcl_item( mcl, Ogre::StringConverter::toString((float) stopwatch->mAccum/totalAccum, 3), col_idx++, row_idx );
                        create_mcl_item( mcl, Ogre::StringConverter::toString((float) stopwatch->mAverage, 3), col_idx++, row_idx );
                        create_mcl_item( mcl, Ogre::StringConverter::toString(stopwatch->mCount), col_idx++, row_idx );
                        create_mcl_item( mcl, Ogre::StringConverter::toString((long) stopwatch->mAccum), col_idx++, row_idx );
                        create_mcl_item( mcl, Ogre::StringConverter::toString((long) stopwatch->mBest), col_idx++, row_idx );
                        create_mcl_item( mcl, Ogre::StringConverter::toString((long) stopwatch->mWorst), col_idx++, row_idx );
                    }
                    
                    CEGUI::Helpers::AutoHeightMultiColumnList(mcl);
                    mStopwatchDataMultiColumnList = mcl;
                }
                
                // octree chunk manager lods
                {
                    CEGUI::MultiColumnList* mcl = static_cast<CEGUI::MultiColumnList*>(layout->createChild(style.scheme + "/MultiColumnList", "VT/Lods/MultiColumn"));
                    CEGUI::UDim w = cegui_reldim( 0.99/3.0 );
                    
                    int col_idx = 0;
                    
                    std::vector< std::string > columnNames =
                    {
                        "lodLevel", "exists", "visible"
                    };
                    for( const std::string& name : columnNames )
                    {
                        mcl->addColumn(name,  col_idx++, w);
                    }
                    
                    mcl->setWidth( cegui_reldim(1.0f) );
                    mcl->setShowHorzScrollbar( false );
                    mcl->setShowVertScrollbar( false );
                    mcl->setUserColumnDraggingEnabled( false );
                    mcl->setUserColumnSizingEnabled( true );

                    std::vector< std::pair<Ogre::uint, Ogre::uint> > counts;
                    mOctreeChunkManager->getLodCounts( counts );
                    
                    for( const std::pair<Ogre::uint, Ogre::uint>& pair : counts )
                    {
                        uint row_idx = mcl->addRow( mcl->getRowCount() + 1 ) ;
                        
                        col_idx = 0;
                        create_mcl_item( mcl, Ogre::StringConverter::toString(row_idx), col_idx++, row_idx );
                        create_mcl_item( mcl, Ogre::StringConverter::toString(pair.first), col_idx++, row_idx );
                        create_mcl_item( mcl, Ogre::StringConverter::toString(pair.second), col_idx++, row_idx );
                    }
                    
                    CEGUI::Helpers::AutoHeightMultiColumnList(mcl);
                    mLodDataMultiColumnList = mcl;
                }
                
                if(mLodStratedy)
                {
                    mAdjustLodsToggleButton = CEGUI::NamedCheckbox(style, layout, "Adjust Lods", true).getToggleButton();
                }
                
                static_cast< CEGUI::TabControl* >( tc )->setSelectedTab("VTTab");
            }
            
            if(mNibbler)
            {
                CEGUI::VerticalLayoutContainer* layout = CEGUI::Helpers::CreateOrRetrieveTabAndLayout(style.scheme, tc, "Nib", false);
                
                CEGUI::Window* win = CEGUI::AutoHeightStaticText(style, layout, "VT/Text").getStaticText();
                win->setWidth( cegui_reldim(1.0) );
                win->setText("Usage : Adds with X and removes with Z keys.");
                
                auto handleEventValueChanged = [&] ( const CEGUI::EventArgs & e ) -> bool
                {
                    const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(e);
                    CEGUI::Slider* slider = static_cast<CEGUI::Slider*>(windowEventArgs.window);
                    mNibbler->setMagnitude(slider->getCurrentValue());
                    return true;
                };
                
                CEGUI::Slider* slider = CEGUI::NamedSlider(style, layout, "Nibbler Magnitude", 100.0).getSlider();
                slider->subscribeEvent(CEGUI::Slider::EventValueChanged, handleEventValueChanged );
                slider->setCurrentValue(mNibbler->getMagnitude());
            }

            
            if( mOctreeChunkSelector )
            {
                CEGUI::VerticalLayoutContainer* layout = CEGUI::Helpers::CreateOrRetrieveTabAndLayout(style.scheme, tc, "Sel", false);
                
                CEGUI::Window* win = CEGUI::AutoHeightStaticText(style, layout, "VT/Text").getStaticText();
                win->setWidth( cegui_reldim(1.0) );
                win->setText("Usage : Select a chunk with the C key.");
                
                mSelectInfo = CEGUI::AutoHeightStaticText(style, layout, "VT/SelectInfo").getStaticText();
                mSelectInfo->setWidth( cegui_reldim(1.0) );
                mSelectInfo->setText("selected chunk : none");
                
                //mModifiedInfo = CEGUI::AutoHeightStaticText(style, layout, "VT/ModifiedInfo").getStaticText();
                //mModifiedInfo->setText("");
                
                CEGUI::PushButton* pb = static_cast<CEGUI::PushButton*>(layout->createChild(style.scheme + "/Button", "VT/Recreate"));
                pb->setWidth( cegui_reldim(1.0) );
                pb->setText("Recursively Re-create Selected Chunk");
                auto handleResetEventClicked = [&] ( const CEGUI::EventArgs & e ) -> bool
                {
                    if( mOctreeChunkSelector->getSelected() )
                    {
                        mOctreeChunkManager->recreate(mOctreeChunkSelector->getSelected(), true);
                    }
                    return true;
                };
                pb->subscribeEvent(CEGUI::PushButton::EventClicked, handleResetEventClicked);
                CEGUI::WindowEventArgs windowEventArgs(pb);
                pb->fireEvent(CEGUI::PushButton::EventClicked, windowEventArgs);
            }
        }
        
        // Influence volumes tab
        if( mOctreeChunkManager && mRaycast && dynamic_cast<VT::OctreeChunkManagerPlusInfluence*>(mOctreeChunkManager) )
        {
            CEGUI::VerticalLayoutContainer* layout = CEGUI::Helpers::CreateOrRetrieveTabAndLayout(style.scheme, tc, "IV", false);
            
            CEGUI::Window* win = CEGUI::AutoHeightStaticText(style, layout, "VT/Text").getStaticText();
            win->setWidth( cegui_reldim(1.0) );
            win->setText("Usage : Place influence points with the V key. Click 'Write' writes the starting influences to the render volumes. To visualise first select a chunk with C key and then 'Propagate'.");
            
            
            mPlaceInfluenceToggleButton = CEGUI::NamedCheckbox( style, layout, "Place Influence Point", false ).getToggleButton();
            mPlaceInfluenceToggleButton->setSelected( true );
            
            mInfluenceValueSlider = CEGUI::NamedAdjustableSlider( style, layout, "Influence Value", -1.0, 1.0, 1.0 ).getSlider();
            
            // write
            {
                CEGUI::PushButton* pb = static_cast<CEGUI::PushButton*>( layout->createChild(style.scheme + "/Button", "IV/Write") );
                pb->setWidth( CEGUI::UDim(1.0, 0) );
                pb->setText("Write");
                auto handleResetEventClicked = [&] ( const CEGUI::EventArgs & e ) -> bool
                {
                    VT::OctreeChunkManager::ChunkIterator it = mOctreeChunkManager->getChunkIterator();
                    while(it.hasMoreElements())
                    {
                        VT::RootOctreeChunkPlusInfluence* chunk = dynamic_cast< VT::RootOctreeChunkPlusInfluence* >( it.getNext() );
                        mInfluenceVolume->write( mSceneMgr, chunk->mInfluence, chunk->mIndex, chunk->mInfluencePoints, mOctreeChunkManager->getSharedParams() );
                    }
                    return true;
                };
                pb->subscribeEvent(CEGUI::PushButton::EventClicked, handleResetEventClicked);
            }
            
            // propagate
            {
                // decay
                {
                    CEGUI::Slider* slider = CEGUI::NamedSlider( style, layout, "Propagation Decay", 1.0 ).getSlider();
                    auto handleEventValueChanged = [&] ( const CEGUI::EventArgs & e ) -> bool
                    {
                        const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(e);
                        CEGUI::Slider* slider = static_cast<CEGUI::Slider*>(windowEventArgs.window);
                        mInfluenceVolume->setDecay(slider->getCurrentValue());
                        return true;
                    };
                    slider->setCurrentValue( mInfluenceVolume->getDecay() );
                    slider->subscribeEvent(CEGUI::Slider::EventValueChanged, handleEventValueChanged );
                }
                
                // momentum
                {
                    CEGUI::Slider* slider = CEGUI::NamedSlider( style, layout, "Propagation Momentum", 1.0 ).getSlider();
                    auto handleEventValueChanged = [&] ( const CEGUI::EventArgs & e ) -> bool
                    {
                        const CEGUI::WindowEventArgs& windowEventArgs = static_cast<const CEGUI::WindowEventArgs&>(e);
                        CEGUI::Slider* slider = static_cast<CEGUI::Slider*>(windowEventArgs.window);
                        mInfluenceVolume->setMomentum(slider->getCurrentValue());
                        return true;
                    };
                    slider->setCurrentValue( mInfluenceVolume->getMomentum() );
                    slider->subscribeEvent(CEGUI::Slider::EventValueChanged, handleEventValueChanged );
                }
                
                mPropagationSpinner = CEGUI::NamedSpinner( style, layout, "Propagation Iterations" ).getSpinner();
                mPropagationSpinner->setMinimumValue(0.0);
                mPropagationSpinner->setMaximumValue(50.0);
                mPropagationSpinner->setStepSize(1.0);
                mPropagationSpinner->setCurrentValue(10.0);
                
                CEGUI::PushButton* pb = static_cast<CEGUI::PushButton*>( layout->createChild(style.scheme + "/Button", "IV/Propagate") );
                pb->setWidth( CEGUI::UDim(1.0, 0) );
                pb->setText("Propagate");
                auto handleResetEventClicked = [&] ( const CEGUI::EventArgs & e ) -> bool
                {
                    int numIterations = mPropagationSpinner->getCurrentValue();
                    for( int i=0; i<numIterations; i++)
                    {
                        VT::OctreeChunkManager::ChunkIterator it = mOctreeChunkManager->getChunkIterator();
                        while(it.hasMoreElements())
                        {
                            VT::RootOctreeChunkPlusInfluence* chunk = dynamic_cast< VT::RootOctreeChunkPlusInfluence* >( it.getNext() );
                            mInfluenceVolume->propagate( mSceneMgr, chunk->mInfluence );

                            // fix texture links post propagation phase
                            chunk->mDisplayMat->getTechnique( "GBuffer" )->getPass(0)->getTextureUnitState(0)->setTexture( chunk->mInfluence->mTexture );
                            chunk->mDisplayDeltaMat->getTechnique( "GBuffer" )->getPass(0)->getTextureUnitState(0)->setTexture( chunk->mInfluence->mTexture );
                            
                            // some bug unless we set the material the texture links don't get fixed
                            chunk->reapplyMaterials();
                        }
                        
                        mPropagationSpinner->setCurrentValue( mPropagationSpinner->getCurrentValue() -1.0 );
                        
                        // viewing the render volumes of a selceted chunk
                        if( mOctreeChunkSelector && mOctreeChunkSelector->getSelected() )
                        {
                            VT::RootOctreeChunkPlusInfluence* selected = dynamic_cast< VT::RootOctreeChunkPlusInfluence* > ( mOctreeChunkSelector->getSelected()->getRoot() );
                            GOOF::DebugTextureManager::getSingleton().removeAllTextures();
                            GOOF::DebugTextureManager::getSingleton().addTexture( selected->getDensity()->mTexture, Ogre::ColourValue(1,0,0,0) );
                            GOOF::DebugTextureManager::getSingleton().addTexture( selected->mInfluence->mTexture, Ogre::ColourValue(1,0,0,0) );
                        }
                        
                        Ogre::Root::getSingleton().renderOneFrame();
                        Ogre::WindowEventUtilities::messagePump();
                    }
                    
                    mPropagationSpinner->setCurrentValue( numIterations );
                    
                    
                    return true;
                };
                pb->subscribeEvent(CEGUI::PushButton::EventClicked, handleResetEventClicked);
            }
        }
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        //DebugTextureManager::getSingleton().addShadowTextures(mSceneMgr, mSceneMgr->getShadowTextureCountPerLightType( Ogre::Light::LT_DIRECTIONAL ));
        
        return success;
    }
    
    
    void VTApplication2::keyPressed(const OIS::KeyEvent &arg)
    {
        ApplicationHierarchy::keyPressed(arg);
        
        if(arg.key == OIS::KC_P)
        {
            mPauseLightAnimation = !mPauseLightAnimation;
        }
        
        if(mOctreeChunkManager && !mLodStratedy)
        {
            // support up to 9 lods!
            static const OIS::KeyCode keys[9] =
            {
                OIS::KC_1, OIS::KC_2, OIS::KC_3, OIS::KC_4, OIS::KC_5, OIS::KC_6, OIS::KC_7, OIS::KC_8, OIS::KC_9
            };
            
            for(int i=0; i<mOctreeChunkManager->getSharedParams()->getLodCount(); i++)
            {
                if(arg.key == keys[i])
                {
                    VT::OctreeChunkManager::ChunkIterator it = mOctreeChunkManager->getChunkIterator();
                    mVisibleLodLevel = i;
                    while(it.hasMoreElements())
                    {
                        VT::OctreeChunk* chunk = it.getNext();
                        setChunkVisible(chunk, mVisibleLodLevel);
                    }
                }
            }
        }
        
        if(mNibbler && mOctreeChunkManager && (arg.key == OIS::KC_X || arg.key == OIS::KC_Z))
        {
            arg.key == OIS::KC_X ? mNibbler->setNibbles(true) : mNibbler->setNibbles(false);
        }
        
        if(arg.key == OIS::KC_V && mOctreeChunkManager && mRaycast && dynamic_cast<VT::OctreeChunkManagerPlusInfluence*>(mOctreeChunkManager) )
        {
            Ogre::Ray ray = InterfaceOIS::getSingleton().getMouseRay(mCamera);
            Ogre::Real nearest;
            VT::OctreeChunk* hitchunk = findNearest( ray, nearest );
            if(hitchunk)
            {
                VT::RootOctreeChunkPlusInfluence* chunk = dynamic_cast< VT::RootOctreeChunkPlusInfluence* >( hitchunk->getRoot() );
                
                if(chunk)
                {
                    VT::InfluencePoint pt;
                    pt.worldPos = ray.getOrigin() + ray.getDirection() * nearest;
                    pt.value = CEGUI::NamedAdjustableSlider::GetSliderCurrentValue( mInfluenceValueSlider );
                    
                    chunk->mInfluencePoints.push_back( pt );
                }
            }
        }
    }
    
    
    void VTApplication2::update(const float deltaTime)
    {
        ApplicationHierarchy::update(deltaTime);
        
        if(mDirLightNode && !mPauseLightAnimation)
        {
            float rotAmt = deltaTime;
            if((mDirLightNode->getOrientation() * Vector3::NEGATIVE_UNIT_Z).dotProduct(Vector3::UNIT_Y) > -0.1)
            {
                rotAmt *= 0.1;
            }
            mDirLightNode->rotate(Vector3::UNIT_X, Ogre::Radian(rotAmt));
        }
        
        
        if(mRaycast)
        {
            Ogre::Ray ray = InterfaceOIS::getSingleton().getMouseRay(mCamera);
            Ogre::Real nearest;
            VT::OctreeChunk* selected = findNearest( ray, nearest );
            if(selected)
            {
                mDebugNode->setPosition( ray.getOrigin() + ray.getDirection() * nearest );
            }
            
            if(mOctreeChunkSelector && getKeyboard()->isKeyDown(OIS::KC_C) )
            {
                if(selected)
                {
                    while(selected->mParent)
                    {
                        selected = selected->mParent;
                    };
                }
                
                VT::OctreeChunk* lastSelected = mOctreeChunkSelector->getSelected();
                mOctreeChunkSelector->select( selected );
                
                if(lastSelected != selected)
                {
                    GOOF::DebugTextureManager::getSingleton().removeAllTextures();
                    
                    if(selected)
                    {
                        Ogre::String info = "selected chunk : " + selected->mIndex.getAsString();
                        mSelectInfo->setText( info );
                        
                        GOOF::DebugTextureManager::getSingleton().addTexture(selected->getDensity()->mTexture, Ogre::ColourValue(1,0,0,0));
                        
                        if( dynamic_cast<VT::OctreeChunkManagerPlusInfluence*>(mOctreeChunkManager) )
                            GOOF::DebugTextureManager::getSingleton().addTexture( dynamic_cast< VT::RootOctreeChunkPlusInfluence* > ( selected->getRoot() )->mInfluence->mTexture, Ogre::ColourValue(1,0,0,0));
                    }
                    else
                    {
                        mSelectInfo->setText( "selected chunk : none" );
                    }
                }
            }
        }
        
        if(mNibbler && mOctreeChunkManager && ( getKeyboard()->isKeyDown(OIS::KC_X) || getKeyboard()->isKeyDown(OIS::KC_Z)))
        {
            if(mLodStratedy)
                mLodStratedy->needsUpdate();
            
            //mDebugNode->setScale(Ogre::Vector3(mNibbler->getAttenuation().x));
            
            Vector3 nibblerPos = mDebugNode->getPosition();
            Ogre::Real nibblerRadius = mNibbler->getAttenuation().x;
            
            /*
            mOctreeChunkManagerSpherePolicy->sphere().setCenter( nibblerPos );
            mOctreeChunkManagerSpherePolicy->sphere().setRadius( nibblerRadius );
            mOctreeChunkManager->setActivePolicy( mOctreeChunkManagerSpherePolicy );
            */

            VT::SharedParams* params = mOctreeChunkManager->getSharedParams();
            
            Ogre::Vector3 marginSize = params->getMargin() * params->getLodVoxelSize(0);
            Ogre::Vector3 chunkSize = params->getLodChunkSize(0);
            
            Vector3 lower = (nibblerPos - nibblerRadius - marginSize)/chunkSize;
            Vector3 upper = (nibblerPos + nibblerRadius + marginSize)/chunkSize;
            
            for(int i=0; i<3; i++)
            {
                lower[i] = Ogre::Math::Floor(lower[i]);
                upper[i] = Ogre::Math::Ceil(upper[i]);
            }
            
            Int3 lowerIdx(lower);
            Int3 upperIdx(upper);
            Int3 idx;
            for(idx.x = lowerIdx.x; idx.x <upperIdx.x; idx.x++)
            {
                for(idx.y = lowerIdx.y; idx.y <upperIdx.y; idx.y++)
                {
                    for(idx.z = lowerIdx.z; idx.z <upperIdx.z; idx.z++)
                    {
                        //Ogre::Vector3 chunkPos = idx.toVector3() * chunkSize;
                        //AxisAlignedBox aab(chunkPos-marginSize, chunkPos + chunkSize + marginSize);
                        
                        AxisAlignedBox aab;
                        params->computeChunkBoundingBox(idx, 0, aab);
                        
                        if(aab.distance(nibblerPos) < nibblerRadius)
                        {
                            VT::OctreeChunk* chunk = mOctreeChunkManager->getChunk(idx, 0);
                            if(chunk)
                            {
                                mNibbler->update(mSceneMgr, chunk->getDensity(), idx, nibblerPos, params);
                                
                                if(!mLodStratedy)
                                {
                                    recreateVisibleChunksWithin(chunk,
                                                                chunk->mParent,
                                                                chunk->mIndex,
                                                                chunk->mLodLevel,
                                                                nibblerPos,
                                                                nibblerRadius);
                                }
                            }
                        }
                    }
                }
            }
        }
        
        /*
         std::vector<VT::OctreeChunk*> modified;
         if(mLodStratedy && mOctreeChunkManager)
         {
         VT::OctreeChunkManager::ChunkIterator it = mOctreeChunkManager->getChunkIterator();
         while(it.hasMoreElements())
         {
         VT::OctreeChunk* chunk = it.getNext();
         if(chunk->mTimeStamp < chunk->rvs->mTimeStamp)
         {
         modified.push_back(chunk);
         }
         }
         }
         */
        
        if(mLodStratedy && mOctreeChunkManager && mAdjustLodsToggleButton->isSelected())
        {
            bool visualChange = false;
            mLodStratedy->adjustLods(mOctreeChunkManager, mCamera->getDerivedPosition(), visualChange);
            if(visualChange)
            {
                mLodStratedy->adjustLodTransitions(mOctreeChunkManager);
            }
            
            mOctreeChunkManager->setActivePolicy(0); // set the active policy back to default
        }
        
        /*
         Ogre::String modifiedStr;
         for( VT::OctreeChunk* chunk : modified )
         {
         if(chunk->mRenderable->getVisible())
         {
         Ogre::RenderOperation op;
         chunk->mRenderable->getRenderToVertexBuffer()->getRenderOperation(op);
         
         modifiedStr += chunk->mIndex.getAsString() + "\n" +
         "list " + Ogre::StringConverter::toString( chunk->mListTrianglesVertexCount ) + "\n" +
         "gen  " + Ogre::StringConverter::toString( op.vertexData->vertexCount/3 ) + "\n";
         }
         }
         if( modifiedStr != "" )
         {
         mModifiedInfo->setText( modifiedStr );
         }
         */
        
        if(mInterfaceCEGUI)
        {
            mEditor->getEditorWindow()->setText("Voxel Terrain " + Ogre::StringConverter::toString(SharedDataAccessor::getRenderWindow()->getStatistics().avgFPS));
            
            CEGUI::MultiColumnList* mcl = mStopwatchDataMultiColumnList;
            
            if( mOctreeChunkManager && mcl )
            {
                Ogre::uint64 totalAccum = 0;
                for( const Stopwatch* stopwatch : mOctreeChunkManager->getStopwatchs() )
                {
                    totalAccum += stopwatch->mAccum;
                }
                
                int row_idx = 0;
                for( const Stopwatch* stopwatch : mOctreeChunkManager->getStopwatchs() )
                {
                    int col_idx = 1;
                    mcl->getItemAtGridReference( CEGUI::MCLGridRef(row_idx, col_idx++) )->setText( Ogre::StringConverter::toString((float) stopwatch->mAccum/totalAccum, 3) );
                    mcl->getItemAtGridReference( CEGUI::MCLGridRef(row_idx, col_idx++) )->setText( Ogre::StringConverter::toString((float) stopwatch->mAverage, 3) );
                    mcl->getItemAtGridReference( CEGUI::MCLGridRef(row_idx, col_idx++) )->setText( Ogre::StringConverter::toString(stopwatch->mCount)) ;
                    mcl->getItemAtGridReference( CEGUI::MCLGridRef(row_idx, col_idx++) )->setText( Ogre::StringConverter::toString((long) stopwatch->mAccum)) ;
                    mcl->getItemAtGridReference( CEGUI::MCLGridRef(row_idx, col_idx++) )->setText( Ogre::StringConverter::toString((long) stopwatch->mBest)) ;
                    mcl->getItemAtGridReference( CEGUI::MCLGridRef(row_idx, col_idx++) )->setText( Ogre::StringConverter::toString((long) stopwatch->mWorst));
                    row_idx++;
                }
                
                mcl->handleUpdatedItemData();
            }
            
            mcl = mLodDataMultiColumnList;
            if( mOctreeChunkManager && mcl )
            {
                std::vector< std::pair<Ogre::uint, Ogre::uint> > counts;
                mOctreeChunkManager->getLodCounts( counts );
                
                int row_idx = 0;
                for( const std::pair<Ogre::uint, Ogre::uint>& pair : counts )
                {
                    int col_idx = 1;
                    mcl->getItemAtGridReference( CEGUI::MCLGridRef(row_idx, col_idx++) )->setText( Ogre::StringConverter::toString(pair.first) );
                    mcl->getItemAtGridReference( CEGUI::MCLGridRef(row_idx, col_idx++) )->setText( Ogre::StringConverter::toString(pair.second) );
                    row_idx++;
                }
                
                mcl->handleUpdatedItemData();
            }


        }
    }
    
    
    void VTApplication2::setChunkVisible(VT::OctreeChunk* chunk, Ogre::uint8 visibleLodLevel)
    {
        if(chunk->mRenderable)
        {
            bool visible = chunk->mLodLevel == visibleLodLevel ? true : false;
            
            if(visible && chunk->mTimeStamp < chunk->getDensity()->mTimeStamp)
            {
                mOctreeChunkManager->recreate(chunk);
            }
            chunk->mRenderable->setVisible(visible);
            
            
            for(int i=0; i<8; i++)
            {
                if(chunk->mChildren[i])
                {
                    setChunkVisible(chunk->mChildren[i], visibleLodLevel);
                }
            }
        }
    }
    
    /*
     void VTApplication::doRayCast(VT::OctreeChunk* chunk, Ogre::uint8 lodlevel, Ogre::Ray& ray, Ogre::Real& nearest)
     {
     if(chunk->mRenderable)
     {
     if(chunk->mRenderable->getVisible())
     {
     //Ogre::Real near, far;
     if(Ogre::Math::intersects(ray, chunk->mRenderable->getBoundingBox(), NULL, NULL))
     {
     mRaycast->update(mSceneMgr, chunk->mRenderable, ray, nearest);
     }
     }
     
     for(int i=0; i<8; i++)
     {
     if(chunk->mChildren[i])
     {
     doRayCast(chunk->mChildren[i], lodlevel+1, ray, nearest);
     }
     }
     }
     }
     */
    
    void VTApplication2::recreateVisibleChunksWithin(VT::OctreeChunk* chunk,
                                                     VT::OctreeChunk* parent,
                                                     const Int3& idx,
                                                     Ogre::uint8 lodLevel,
                                                     const Ogre::Vector3& pt,
                                                     const Ogre::Real& radius)
    {
        VT::SharedParams* params = mOctreeChunkManager->getSharedParams();
        
        Ogre::Vector3 chunkSize = params->getLodChunkSize(lodLevel);
        Ogre::Vector3 marginSize = params->getMargin() * params->getLodVoxelSize(lodLevel);
        
        bool isTerminal = lodLevel == params->getLodCount()  ? true : false;
        
        Ogre::Vector3 chunkPos = idx.toVector3() * chunkSize;
        AxisAlignedBox aab(chunkPos-marginSize, chunkPos + chunkSize + marginSize);
        
        if(aab.distance(pt) < radius && !isTerminal)
        {
            if((chunk && chunk->mRenderable && chunk->mRenderable->isVisible() == false))
            {
                // don't recreate non visible chunks
            }
            else if(chunk)
            {
                mOctreeChunkManager->recreate(chunk, false);
            }
            else
            {
                // previously empty chunk may now be filled
                chunk = mOctreeChunkManager->create(idx, lodLevel, parent, false);
            }
            
            if(chunk)
            {
                for(int i=0; i<8; i++)
                {
                    recreateVisibleChunksWithin(chunk->mChildren[i], chunk, idx*2 + mOctreeChunkManager->getQuadrant(i), lodLevel+1, pt, radius);
                }
            }
        }
        else
        {
            // outside radius of effect
        }
    }
    
    
}


