#include "GOOFVTOctreeChunk.h"
#include "GOOFRenderTexture.h"
#include "GOOFProceduralRenderable.h"
#include "GOOFNameGenerator.h"
#include "GOOFStopwatch.h"
#include <OgreMaterialManager.h>
#include <OgreWireBoundingBox.h>

namespace GOOF
{
    namespace VT
    {
        OctreeChunk::OctreeChunk()  : mParent(0), mRenderable(0), mTimeStamp(0), mWireBoundingBox(0)
        {
            for(int i=0; i<8; i++)
            {
                mChildren[i] = 0;
            }
            for(int i=0; i<6; i++)
            {
                mTransition[i] = 0;
                mSibling[i] = 0;
            }
        }
        
        OctreeChunk::~OctreeChunk()
        {
            if(mWireBoundingBox)
            {
                mWireBoundingBox->getParentSceneNode()->detachObject( mWireBoundingBox );
                OGRE_DELETE mWireBoundingBox;
            }
            if(mRenderable)
            {
                mRenderable->_getManager()->destroyMovableObject(mRenderable);
            }
            for(int i=0; i<6; i++)
            {
                if(mSibling[i])
                {
                    int reverse = (i%2 == 1) ? i -1 : i + 1;
                    mSibling[i]->mSibling[reverse] = NULL;
                }
                if(mTransition[i])
                {
                    mTransition[i]->getParentSceneNode()->detachObject(mTransition[i]);
                    mTransition[i]->_getManager()->destroyMovableObject(mTransition[i]);
                }
            }
            for(int i=0; i<8; i++)
            {
                if(mChildren[i])
                {
                    delete mChildren[i];
                }
            }
        }
        
        OctreeChunk* OctreeChunk::getRoot()
        {
            if(!mParent)
                return this;
            
            OctreeChunk* parent = mParent;
            while(parent->mParent)
            {
                parent = parent->mParent;
            }
            return parent;
        }
        
        Ogre::WireBoundingBox* OctreeChunk::getWireBoundingBox()
        {
            if(mWireBoundingBox == 0 && mRenderable)
            {
                mWireBoundingBox = OGRE_NEW Ogre::WireBoundingBox();
                
                Ogre::AxisAlignedBox shrinked( mRenderable->getBoundingBox() );
                shrinked.setMaximum( shrinked.getMaximum() - Ogre::Vector3::UNIT_SCALE * mLodLevel );
                shrinked.setMinimum( shrinked.getMinimum() + Ogre::Vector3::UNIT_SCALE * mLodLevel );
                
                mWireBoundingBox->setupBoundingBox( shrinked );
                mWireBoundingBox->setMaterial("Debug/BaseWhite");
                
                getSceneNode()->attachObject( mWireBoundingBox );
            }
            return mWireBoundingBox;
        }
        
        void OctreeChunk::setRenderableVisible( bool visible )
        {
            if(mRenderable)
                mRenderable->setVisible(visible);
        }
        
        void OctreeChunk::hideAllChildren()
        {
            for(int i=0; i<8; i++)
            {
                if(mChildren[i])
                {
                    mChildren[i]->setRenderableVisible(false);
                    mChildren[i]->hideAllChildren();
                    
                    for(ProceduralRenderable* transition : mChildren[i]->mTransition)
                    {
                        if(transition)
                            transition->setVisible(false);
                    }
                }
            }
        }
        
        void OctreeChunk::reapplyMaterials()
        {
            if( mRenderable )
            {
                mRenderable->setMaterial( getDisplayMat() );
            }
            
            for( int i=0; i<6; i++ )
            {
                if( mTransition[i] )
                {
                    mTransition[i]->setMaterial( getDisplayDeltaMat() );
                }
            }
            
            for( int i=0; i<8; i++ )
            {
                if( mChildren[i] )
                {
                    mChildren[i]->reapplyMaterials();
                }
            }
        }
        
        OctreeChunkManager::OctreeChunkManager(Ogre::SceneManager* sceneMgr,
                                               SharedParams* sharedParams,
                                               Ogre::Camera* camera,
                                               const Ogre::String& densityMaterialName,
                                               const Ogre::String& displayMaterialName,
                                               const Ogre::String& displayDeltaMaterialName) :
            mSceneMgr(sceneMgr),
            mSharedParams(sharedParams),
            mCamera(camera)
        {
            mDefaultPolicy = new OctreeChunkManagerPolicy(mSharedParams, false);
            mActivePolicy = mDefaultPolicy;
            
            mDensityMat = Ogre::MaterialManager::getSingleton().load(densityMaterialName, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME).staticCast<Ogre::Material>();
            mDisplayMat = Ogre::MaterialManager::getSingleton().load(displayMaterialName, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME).staticCast<Ogre::Material>();
            mDisplayDeltaMat = Ogre::MaterialManager::getSingleton().load(displayDeltaMaterialName, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME).staticCast<Ogre::Material>();

            mRegularSeed = new VT::RegularSeed(mSceneMgr, sharedParams);
            mListTriangles = new VT::ListTriangles(mSceneMgr, mRegularSeed->mSeedObject, sharedParams);
            mGenTriangles = new VT::GenTriangles( VT::RegularTriTableTexture().mTriTable );
            mGenTrianglesDelta = new VT::GenTrianglesDelta( VT::RegularTriTableTexture().mTriTable );
            
            mTransitionSeed = new VT::TransitionSeed(mSceneMgr, sharedParams);
            mTransitionListTriangles = new VT::TransitionListTriangles(mSceneMgr, mTransitionSeed->mSeedObject, sharedParams);
            mTransitionGenTriangles = new VT::TransitionGenTriangles( VT::TransitionTriTableTexture().mTriTable );
            
            std::vector< std::string > names =
            {
                "rvs",
                "List",
                "Gen",
                "Gen Delta",
                "List Trans",
                "Gen Trans"
            };
            
            for( const auto& name : names )
            {
                Stopwatch* stopwatch = new Stopwatch();
                stopwatch->name = name;
                mStopwatches.push_back( stopwatch );
            }

        }
        
        OctreeChunkManager::~OctreeChunkManager()
        {
            for (ChunkMap::iterator it = mChunks.begin(); it != mChunks.end(); ++it)
            {
                delete it->second;
            }
            mChunks.clear();

            delete mListTriangles;
            delete mGenTriangles;
            delete mGenTrianglesDelta;
            delete mTransitionListTriangles;
            delete mTransitionGenTriangles;
            delete mRegularSeed;
            delete mTransitionSeed;
            delete mDefaultPolicy;
        }
        
        OctreeChunk* OctreeChunkManager::createRoot( Ogre::SceneNode* sceneNode, RenderVolumeWithTimeStamp* rvs )
        {
            RootOctreeChunk* root = new RootOctreeChunk( );
            root->mSceneNode = sceneNode;
            root->mDensity = rvs;
            root->mDisplayMat = mDisplayMat;
            root->mDisplayDeltaMat = mDisplayDeltaMat;
            
            return root;
        }

        OctreeChunk* OctreeChunkManager::createEmpty(const Int3& index, Ogre::uint8 lodLevel, OctreeChunk* parent, bool recursive)
        {
            assert(parent);
            
            bool isRoot = parent == 0 ? true : false;
            bool isTerminal = lodLevel == mSharedParams->getLodCount() - 1 ? true : false;

            OctreeChunk* chunk = isRoot ? createRoot(mSceneMgr->getRootSceneNode()->createChildSceneNode(), 0) : new OctreeChunk();
            chunk->mIndex = index;
            chunk->mLodLevel = lodLevel;
            chunk->mTimeStamp = 0;
            chunk->mParent = parent;
            chunk->mListTrianglesVertexCount = 0;
            parent->mChildren[ getQuadrantIndex( getQuadrant(index) ) ] = chunk;
            
            for(int i=0; i<6; i++)
                chunk->mTransitionTimeStamp[i] = 0;
            
            static const GOOF::Int3 SIBLING_FACE[6] =
            {
                GOOF::Int3::UNIT_X,
                GOOF::Int3::NEGATIVE_UNIT_X,
                GOOF::Int3::UNIT_Y,
                GOOF::Int3::NEGATIVE_UNIT_Y,
                GOOF::Int3::UNIT_Z,
                GOOF::Int3::NEGATIVE_UNIT_Z
            };
            
            for(int i=0; i<6; i++)
            {
                Int3 siblingIndex = index + SIBLING_FACE[i];
                OctreeChunk* sibling = getChunk(siblingIndex, lodLevel);
                
                if(sibling)
                {
                    sibling->mSibling[i] = chunk;
                    
                    int reverse = (i%2 == 1) ? i -1 : i + 1;
                    chunk->mSibling[reverse] = sibling;
                }
            }
            
            if(recursive && !isTerminal)
            {
                Int3 quadrant;
                for(quadrant.x=0; quadrant.x<2; quadrant.x++)
                {
                    for(quadrant.y=0; quadrant.y<2; quadrant.y++)
                    {
                        for(quadrant.z=0 ; quadrant.z<2; quadrant.z++)
                        {
                            create(index*2 + quadrant, lodLevel+1, chunk, recursive);
                        }
                    }
                }
            }
            
            return chunk;
        }
        
        OctreeChunk* OctreeChunkManager::create(const Int3& index, Ogre::uint8 lodLevel, OctreeChunk* parent, bool recursive)
        {
            if( !mActivePolicy->doCreate(index, lodLevel) )
            {
                return 0;
            }
            
            const bool isRoot = parent == 0 ? true : false;
            const bool isTerminal = lodLevel == mSharedParams->getLodCount() - 1 ? true : false;
            
            mSharedParams->setRegularParams(index, lodLevel);

            RenderVolumeWithTimeStamp* rvs = 0;
            
            if(isRoot)
            {
                Ogre::uint dim = mSharedParams->getRenderVolumeSize();
                rvs = new RenderVolumeWithTimeStamp(mCamera, NameGenerator::Next("RenderVolume"), Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, dim, dim, dim, Ogre::PF_FLOAT16_R);
                
                mStopwatches[0]->start();
                rvs->render(mSceneMgr, mDensityMat->getTechnique(0)->getPass(0));
                mStopwatches[0]->stop();
            }
            else
            {
                rvs = parent->getDensity();
            }
            
            mListTriangles->setDensity(rvs->mTexture);
            mListTriangles->update(mSceneMgr, mStopwatches[1]);
            
            const bool empty = mListTriangles->isEmpty();
            const bool doCulling = mActivePolicy->doCulling();
            
            if(!isRoot && empty && doCulling)
            {
                return 0;
            }
            
            OctreeChunk* chunk = isRoot ? createRoot(mSceneMgr->getRootSceneNode()->createChildSceneNode(), rvs) : new OctreeChunk();
            chunk->mIndex = index;
            chunk->mLodLevel = lodLevel;
            chunk->mTimeStamp = Ogre::Root::getSingleton().getTimer()->getMicroseconds();
            chunk->mListTrianglesVertexCount = mListTriangles->vertexCount();

            if(!isRoot)
            {
                chunk->mParent = parent;
                parent->mChildren[ getQuadrantIndex( getQuadrant(index) ) ] = chunk;
            }
            else
            {
                // add root chunks to the map
                mChunks.insert(std::pair<Int3, OctreeChunk*>(index, chunk));
            }
            
            static const GOOF::Int3 SIBLING_FACE[6] =
            {
                GOOF::Int3::UNIT_X,
                GOOF::Int3::NEGATIVE_UNIT_X,
                GOOF::Int3::UNIT_Y,
                GOOF::Int3::NEGATIVE_UNIT_Y,
                GOOF::Int3::UNIT_Z,
                GOOF::Int3::NEGATIVE_UNIT_Z
            };
            
            for(int i=0; i<6; i++)
            {
                Int3 siblingIndex = index + SIBLING_FACE[i];
                OctreeChunk* sibling = getChunk(siblingIndex, lodLevel);
                
                if(sibling)
                {
                    sibling->mSibling[i] = chunk;
                    
                    int reverse = (i%2 == 1) ? i -1 : i + 1;
                    chunk->mSibling[reverse] = sibling;
                }
            }
            
            const bool doChildren = recursive && mActivePolicy->doChildren(index, lodLevel, empty);
            
            if(empty && !doChildren)
            {
                return chunk;
            }
            else if(!empty)
            {
                ProceduralRenderable* renderable = static_cast<ProceduralRenderable*>(mSceneMgr->createMovableObject(ProceduralRenderableFactory::FACTORY_TYPE_NAME));
                chunk->mRenderable = renderable;
                
                // both MovableObject and Renderable have UserObjectBindings
                ((Ogre::MovableObject*) renderable)->getUserObjectBindings().setUserAny(typeid(OctreeChunk).name(), Ogre::Any(chunk));
                
                if(isTerminal)
                {
                    mGenTriangles->setDensity(rvs->mTexture);
                    mGenTriangles->update(mSceneMgr, mListTriangles->mList, renderable, mStopwatches[2]);
                    renderable->setMaterial( chunk->getDisplayMat() );
                    
                    /*
                    if(index == Int3(0,0,2))
                    {
                        mListTriangles->log(1, index.getAsString() );
                        mGenTriangles->log(renderable, 0, index.getAsString() );
                    }
                    */
                }
                else
                {
                    mGenTrianglesDelta->setDensity(rvs->mTexture);
                    mGenTrianglesDelta->update(mSceneMgr, mListTriangles->mList, renderable, mStopwatches[3]);
                    renderable->setMaterial( chunk->getDisplayDeltaMat() );
                }
                
                Ogre::AxisAlignedBox aab;
                mSharedParams->computeChunkBoundingBox(index, lodLevel, aab);
                renderable->setBoundingBox(aab);
                
                renderable->setVisible(false);
                chunk->getSceneNode()->attachObject(renderable);
                
                // vertex shader deltaFacesPos and deltaFacesNeg
                renderable->setCustomParameter(0, Ogre::Vector4(0,0,0,0));
                renderable->setCustomParameter(1, Ogre::Vector4(0,0,0,0));
                
                // fragment shader highlight (custom params can have gaps in there indexs)
                //renderable->setCustomParameter(10, Ogre::Vector4(0,0,0,0));
            }
            
            if(!isTerminal && doChildren)
            {
                Int3 quadrant;
                for(quadrant.x=0; quadrant.x<2; quadrant.x++)
                {
                    for(quadrant.y=0; quadrant.y<2; quadrant.y++)
                    {
                        for(quadrant.z=0 ; quadrant.z<2; quadrant.z++)
                        {
                            create(index*2 + quadrant, lodLevel+1, chunk, recursive);
                        }
                    }
                }
            }
            
            return chunk;
        }
        
        bool OctreeChunkManager::recreate(OctreeChunk* chunk, bool recursive)
        {
            const Int3& index = chunk->mIndex;
            const Ogre::uint8 lodLevel = chunk->mLodLevel;
            
            //std::cout << "recreate " << index.x << "  " << index.y << "  " << index.z << "  " << lodLevel << std::endl;

            if( !mActivePolicy->doCreate(index, lodLevel) )
            {
                return false;
            }
            
            mSharedParams->setRegularParams(index, lodLevel);
            
            const bool isTerminal = lodLevel == mSharedParams->getLodCount() - 1 ? true : false;

            RenderVolume* rvs = chunk->getDensity();
            
            mListTriangles->setDensity(rvs->mTexture);
            mListTriangles->update(mSceneMgr, mStopwatches[1]);
            chunk->mTimeStamp = Ogre::Root::getSingleton().getTimer()->getMicroseconds();
            chunk->mListTrianglesVertexCount = mListTriangles->vertexCount();

            ProceduralRenderable* renderable = chunk->mRenderable;
            
            const bool empty = mListTriangles->isEmpty();
            
            if(empty && renderable)
            {
                chunk->mRenderable->getRenderToVertexBuffer()->reset();
                chunk->getSceneNode()->detachObject(renderable);
            }
            
            if(!empty)
            {
                if(!renderable)
                {
                    renderable = static_cast<ProceduralRenderable*>(mSceneMgr->createMovableObject(ProceduralRenderableFactory::FACTORY_TYPE_NAME));
                    chunk->mRenderable = renderable;
                    
                    // both MovableObject and Renderable have UserObjectBindings
                    ((Ogre::MovableObject*) renderable)->getUserObjectBindings().setUserAny(typeid(OctreeChunk).name(), Ogre::Any(chunk));
                                        
                    Ogre::AxisAlignedBox aab;
                    mSharedParams->computeChunkBoundingBox(index, lodLevel, aab);
                    renderable->setBoundingBox(aab);
                    
                    renderable->setVisible(false);
                    chunk->getSceneNode()->attachObject(renderable);
                }
                else if(!renderable->isAttached())
                {
                    chunk->getSceneNode()->attachObject(renderable);
                }
                
                if(isTerminal)
                {
                    mGenTriangles->setDensity(rvs->mTexture);
                    mGenTriangles->update(mSceneMgr, mListTriangles->mList, renderable, mStopwatches[2]);
                    renderable->setMaterial( chunk->getDisplayMat() );
                    //mListTriangles->log(1, "recreate " + index.getAsString() );
                    //mGenTriangles->log(renderable, 0, "recreate " + index.getAsString() );
                }
                else
                {
                    mGenTrianglesDelta->setDensity(rvs->mTexture);
                    mGenTrianglesDelta->update(mSceneMgr, mListTriangles->mList, renderable, mStopwatches[3]);
                    renderable->setMaterial( chunk->getDisplayDeltaMat() );
                }
            }
            
            const bool doChildren = recursive && mActivePolicy->doChildren(index, lodLevel, empty);
            
            if(!isTerminal && doChildren)
            {
                for(int i=0; i<8; i++)
                {
                    if(chunk->mChildren[i])
                    {
                        recreate(chunk->mChildren[i], recursive);
                    }
                    else
                    {
                        create(index*2 + getQuadrant(i), lodLevel+1, chunk, recursive);
                    }
                }
            }
            
            return true;
        }
    
        void OctreeChunkManager::createTransition(OctreeChunk* chunk, Ogre::uint8 face, Ogre::uint8 side)
        {
            mSharedParams->setRegularParams(chunk->mIndex, chunk->mLodLevel);
            mSharedParams->setTransitionParams(face, side);
            
            mTransitionListTriangles->setDensity(chunk->getDensity()->mTexture);
            mTransitionListTriangles->update(mSceneMgr, mStopwatches[4]);
            
            if(mTransitionListTriangles->isEmpty())
            {
                return;
            }
            
            Ogre::uint8 idx = face*2 + side;
            
            ProceduralRenderable* renderable = static_cast<ProceduralRenderable*>(mSceneMgr->createMovableObject(ProceduralRenderableFactory::FACTORY_TYPE_NAME));
            chunk->mTransition[idx] = renderable;
            
            mTransitionGenTriangles->update(mSceneMgr, mTransitionListTriangles->mList, renderable, mStopwatches[5]);
            chunk->mTransitionTimeStamp[idx] = Ogre::Root::getSingleton().getTimer()->getMicroseconds();

            renderable->setMaterial( chunk->getDisplayMat() );
            
            const Ogre::AxisAlignedBox& aab = chunk->mRenderable->getBoundingBox();
            renderable->setBoundingBox(aab); // could improve this
            
            chunk->getSceneNode()->attachObject(renderable);
        }
        
        void OctreeChunkManager::recreateTransition(OctreeChunk* chunk, Ogre::uint8 face, Ogre::uint8 side)
        {
            mSharedParams->setRegularParams(chunk->mIndex, chunk->mLodLevel);
            mSharedParams->setTransitionParams(face, side);
            
            mTransitionListTriangles->setDensity(chunk->getDensity()->mTexture);
            mTransitionListTriangles->update(mSceneMgr, mStopwatches[4]);
            Ogre::uint8 idx = face*2 + side;
            chunk->mTransitionTimeStamp[idx] = Ogre::Root::getSingleton().getTimer()->getMicroseconds();
            
            ProceduralRenderable* renderable = chunk->mTransition[idx];
            
            if(mTransitionListTriangles->isEmpty())
            {
                chunk->getSceneNode()->detachObject(renderable);
                return;
            }
            
            mTransitionGenTriangles->update(mSceneMgr, mTransitionListTriangles->mList, renderable, mStopwatches[5]);

            if(!renderable->isAttached())
            {
                chunk->getSceneNode()->attachObject(renderable);
            }
        }
        
        void OctreeChunkManager::destroy(OctreeChunk* chunk)
        {
            for (ChunkMap::iterator it = mChunks.begin(); it != mChunks.end(); ++it)
            {
                if(it->second == chunk)
                {
                    delete it->second;
                    mChunks.erase(it);
                    break;
                }
            }
        }
        
        OctreeChunk* OctreeChunkManager::getChunk(const Int3& index, const Ogre::uint8 lodLevel)
        {
            OctreeChunk* chunk;
            
            Ogre::Vector3 v = index.toVector3() / (1<<lodLevel);
            v.x = Ogre::Math::Floor(v.x);
            v.y = Ogre::Math::Floor(v.y);
            v.z = Ogre::Math::Floor(v.z);
            
            Int3 cell;
            cell.fromVector3(v);
            
            ChunkMap::iterator it = mChunks.find(cell);
            if(it != mChunks.end())
            {
                chunk = it->second;
            }
            else
            {
                return NULL;
            }
            
            Int3 subpart = index - (cell * (1<<lodLevel));
            
            for(int i=0; i<lodLevel; i++)
            {
                // subpart / ... 8 4 2 1
                v = subpart.toVector3() / (1 << (lodLevel-i-1));
                v.x = Ogre::Math::Floor(v.x);
                v.y = Ogre::Math::Floor(v.y);
                v.z = Ogre::Math::Floor(v.z);
                
                cell.fromVector3(v);
                
                subpart -= cell * (1 << (lodLevel-i-1));
                
                chunk = chunk->mChildren[ getQuadrantIndex(cell) ];
                
                if(!chunk)
                {
                    return NULL;
                }
            }
            
            return chunk;
        }
        
        OctreeChunkManager::ChunkIterator OctreeChunkManager::getChunkIterator()
        {
            return ChunkIterator(mChunks.begin(), mChunks.end());
        }
        
        void OctreeChunkManager::logStopwatches(Ogre::String& log)
        {
            Ogre::uint64 totalAccum = 0;
            
            for( const Stopwatch* stopwatch : mStopwatches )
            {
                totalAccum += stopwatch->mAccum;
            }
            
            for( const Stopwatch* stopwatch : mStopwatches )
            {
                log += stopwatch->name + std::string(12 - stopwatch->name.size(), ' ') + Ogre::StringConverter::toString((float) stopwatch->mAccum/totalAccum, 3, 8, ' ') + "% ";
                stopwatch->catToString(log);
                log += "\n";
            }
        }
        
        void OctreeChunkManager::logLods(Ogre::String& log)
        {
            std::vector< std::pair<Ogre::uint, Ogre::uint> > counts;
            getLodCounts( counts );
            
            log += "(lodLevel, exists, visible) ";
            for(int lodLevel = 0; lodLevel < counts.size(); lodLevel++)
            {
                log += "(" + Ogre::StringConverter::toString(lodLevel) + ", " + Ogre::StringConverter::toString(counts[lodLevel].first) + ", " + Ogre::StringConverter::toString(counts[lodLevel].second) + ")";
                if(lodLevel < counts.size() - 1)
                    log += ", ";
                else
                    log += "\n";
            }
        }
        
        void OctreeChunkManager::getLodCounts(std::vector< std::pair<Ogre::uint, Ogre::uint> >& counts)
        {
            std::function< void (VT::OctreeChunk* chunk, std::vector< std::pair<Ogre::uint, Ogre::uint> >& counts) > do_counts;
            do_counts = [ &do_counts ]( VT::OctreeChunk* chunk, std::vector< std::pair<Ogre::uint, Ogre::uint> >& counts )
            {
                if(chunk)
                {
                    if(chunk->mRenderable)
                    {
                        counts[chunk->mLodLevel].first++;
                        
                        if(chunk->mRenderable->isVisible())
                        {
                            counts[chunk->mLodLevel].second++;
                        }
                    }
                    
                    for(int childIdx=0; childIdx<8; childIdx++)
                    {
                        do_counts(chunk->mChildren[childIdx], counts);
                    }
                }
            };
            
            counts.resize( mSharedParams->getLodCount() );
            VT::OctreeChunkManager::ChunkIterator it = getChunkIterator();
            while(it.hasMoreElements())
            {
                do_counts(it.getNext(), counts);
            }
        }

        
        SimpleLodStratedy::SimpleLodStratedy(SharedParams* sharedParams) :
        mSharedParams(sharedParams),
        mLastPos(Ogre::Math::POS_INFINITY),
        mNeedsUpdate(true)
        {
            calculateSplitPoints(2.0, 500.0, 0.9f);
        }
        
        void SimpleLodStratedy::calculateSplitPoints(Ogre::Real nearDist, Ogre::Real farDist, Ogre::Real lambda)
        {
            mSplitPoints.resize(mSharedParams->getLodCount());
            
            mSplitPoints[0] = farDist;
            for (size_t i = 1; i < mSplitPoints.size(); i++)
            {
                Ogre::Real fraction = (Ogre::Real)i / (Ogre::Real) mSplitPoints.size();
                Ogre::Real splitPoint = lambda * nearDist * Ogre::Math::Pow(farDist / nearDist, fraction) +
                (1.0f - lambda) * (nearDist + fraction * (farDist - nearDist));
                
                mSplitPoints[mSharedParams->getLodCount() - i] = splitPoint;
            }
            
            mSplitPoints[mSplitPoints.size() - 1] = 0.0;
        }
        
        void SimpleLodStratedy::adjustLods(OctreeChunkManager* chunkMgr, const Ogre::Vector3& pos, bool& visualChange, const Ogre::Real& proportion)
        {
            if(pos.distance(mLastPos) > mSharedParams->getLodChunkSize(mSharedParams->getLodCount() - 1).x * proportion || mNeedsUpdate)
            {
                VT::OctreeChunkManager::ChunkIterator it = chunkMgr->getChunkIterator();
                while(it.hasMoreElements())
                {
                    VT::OctreeChunk* chunk = it.getNext();
                    adjustLod(pos, chunkMgr, chunk, visualChange);
                }
                
                mLastPos = pos;
                mNeedsUpdate = false;
            }
        }
        
        void SimpleLodStratedy::adjustLodTransitions(OctreeChunkManager* chunkMgr)
        {
            VT::OctreeChunkManager::ChunkIterator it = chunkMgr->getChunkIterator();
            while(it.hasMoreElements())
            {
                VT::OctreeChunk* chunk = it.getNext();
                adjustLodTransitions(chunkMgr, chunk);
            }
        }
        
        bool SimpleLodStratedy::continueBranch(OctreeChunk* chunk, const Ogre::Vector3& pos)
        {
            const Ogre::AxisAlignedBox& aab = chunk->mRenderable->getBoundingBox();
            Ogre::Vector3 max = aab.getMaximum() - pos;
            Ogre::Vector3 min = aab.getMinimum() - pos;
            
            const Ogre::Real split = mSplitPoints[ chunk->mLodLevel ];
            if((std::abs(max.x) <= split || std::abs(min.x) <= split) &&
               (std::abs(max.y) <= split || std::abs(min.y) <= split) &&
               (std::abs(max.z) <= split || std::abs(min.z) <= split))
            {
                return true;
            }
            return false;
        }
        
        void SimpleLodStratedy::adjustLod(const Ogre::Vector3& pos, OctreeChunkManager* chunkMgr, OctreeChunk* chunk, bool& visualChange )
        {
            if(chunk->mRenderable)
            {
                if( continueBranch(chunk, pos)  )
                {
                    chunk->setRenderableVisible(false);
                    for(int i=0; i<6; i++)
                    {
                        if(chunk->mTransition[i])
                        {
                            chunk->mTransition[i]->setVisible(false);
                        }
                    }
                    
                    for(int childIdx=0; childIdx<8; childIdx++)
                    {
                        if(chunk->mTimeStamp < chunk->getDensity()->mTimeStamp && !chunk->mChildren[childIdx])
                        {
                            if(chunkMgr->create(chunk->mIndex*2 + chunkMgr->getQuadrant(childIdx), chunk->mLodLevel+1, chunk, false))
                            {
                                visualChange = true;
                            }
                        }
                        
                        if(chunk->mChildren[childIdx])
                        {
                            adjustLod(pos, chunkMgr, chunk->mChildren[childIdx], visualChange);
                        }
                    }
                }
                else
                {
                    if(!chunk->mRenderable->isVisible())
                        visualChange = true;
                    
                    if(chunk->mTimeStamp < chunk->getDensity()->mTimeStamp)
                    {
                        visualChange = true;
                        chunkMgr->recreate(chunk);
                    }
                    
                    chunk->setRenderableVisible(true);
                    chunk->hideAllChildren();
                }
            }
            
            else if(chunk->mTimeStamp < chunk->getDensity()->mTimeStamp)
            {
                if(chunkMgr->recreate(chunk, true))
                {
                    visualChange = true;
                    adjustLod(pos, chunkMgr, chunk, visualChange);
                }
            }
        }
        
        
        void SimpleLodStratedy::adjustLodTransitions(OctreeChunkManager* chunkMgr, OctreeChunk* chunk)
        {
            if(chunk->mRenderable)
            {
                if(chunk->mRenderable->isVisible())
                {
                    for(int side=0; side<2; side++)
                    {
                        Ogre::Vector4 deltaFaces;
                        
                        for(int face=0; face<3; face++)
                        {
                            Ogre::uint8 i = face*2 + side;	// transition index
                            OctreeChunk* sibling = chunk->mSibling[i];
                            
                            while( sibling && (!sibling->mRenderable || !sibling->mRenderable->isVisible() ) )
                            {
                                sibling = sibling->mParent;
                            }

                            // transition to a higher lod
                            if( !sibling && chunk->mSibling[i])
                            {
                                deltaFaces[face] = 1;
                                
                                if(!chunk->mTransition[i])
                                {
                                    chunkMgr->createTransition(chunk, face, side);
                                }
                                else if(chunk->mTransitionTimeStamp[i] < chunk->getDensity()->mTimeStamp)
                                {
                                    chunkMgr->recreateTransition(chunk, face, side);
                                }
                                
                                if(chunk->mTransition[i])
                                    chunk->mTransition[i]->setVisible(true);
                            }
                            
                            /*
                            // transition from this lod to a sibling higher lod with a gap of 2 lod levels between them
                            // a transition to lower lod (which currently don't have)
                            else if( (sibling && sibling != chunk->mSibling[i])
                                    &&
                                    ((int)chunk->mLodLevel - (int)sibling->mLodLevel > 1) )
                            {
                                deltaFaces[face] = 1;
                                if(chunk->mTransition[i])
                                {
                                    chunk->mTransition[i]->setVisible(false);
                                }
                                //chunk->mRenderable->setCustomParameter(10, Ogre::Vector4(0.5,0.0,0.0,0)); // debug
                            }
                            */
                            
                            else
                            {
                                deltaFaces[face] = 0;
                                if(chunk->mTransition[i])
                                {
                                    chunk->mTransition[i]->setVisible(false);
                                }
                            }
                        }
                        chunk->mRenderable->setCustomParameter(side, deltaFaces);
                    }
                }
                else
                {
                    for (OctreeChunk* child : chunk->mChildren)
                    {
                        if(child)
                        {
                            adjustLodTransitions(chunkMgr, child);
                        }
                    }
                }
            }
        }
        
        
        SmartLodStratedy::SmartLodStratedy(SharedParams* sharedParams) : GOOF::VT::SimpleLodStratedy(sharedParams)
        {
            mLodPolyStats.resize(sharedParams->getLodCount());
        }
        
        bool SmartLodStratedy::continueBranch(OctreeChunk* chunk, const Ogre::Vector3& pos)
        {
            if( SimpleLodStratedy::continueBranch(chunk, pos) )
            {
                mPassedSplitIdx = chunk->mLodLevel;
                return true;
            }
            
            else if(mPassedSplitIdx + 1 == (int) chunk->mLodLevel)
            {
                PolyStats& stats = mLodPolyStats[chunk->mLodLevel];
                if(chunk->mListTrianglesVertexCount > stats.mean + stats.sigma &&
                   chunk->mLodLevel + 1 < mLodPolyStats.size() )
                {
                    return true;
                }
            }
            
            return false;
        }

        
        void SmartLodStratedy::adjustLods(OctreeChunkManager* chunkMgr, const Ogre::Vector3& pos, bool& visualChange, const Ogre::Real& proportion)
        {
            SimpleLodStratedy::adjustLods(chunkMgr, pos, visualChange, proportion);
            
            if(visualChange)
            {
                for(PolyStats& stats : mLodPolyStats)
                {
                    stats.n = 0;
                    stats.sum = 0;
                    stats.sum_xi_minus_mean_sq = 0.0;
                }
                
                std::function< void (VT::OctreeChunk* chunk) > stats_n_and_sum;
                stats_n_and_sum = [&stats_n_and_sum,
                                   &mLodPolyStats = this->mLodPolyStats](VT::OctreeChunk* chunk)
                {
                    if(chunk->mRenderable)
                    {
                        PolyStats& stats = mLodPolyStats[chunk->mLodLevel];
                        stats.n++;
                        stats.sum += chunk->mListTrianglesVertexCount;
                    }

                    for(OctreeChunk* child : chunk->mChildren)
                    {
                        if(child)
                        {
                            stats_n_and_sum(child);
                        }
                    }
                };
                
                VT::OctreeChunkManager::ChunkIterator it = chunkMgr->getChunkIterator();
                while(it.hasMoreElements())
                {
                    VT::OctreeChunk* chunk = it.getNext();
                    stats_n_and_sum(chunk);
                }
                
                for(PolyStats& stats : mLodPolyStats)
                {
                    stats.mean = stats.sum / stats.n;
                }
                
                
                std::function< void (VT::OctreeChunk* chunk) > stats_sum_xi_minus_mean_sq;
                stats_sum_xi_minus_mean_sq = [&stats_sum_xi_minus_mean_sq,
                                              &mLodPolyStats = this->mLodPolyStats](VT::OctreeChunk* chunk)
                {
                    if(chunk->mRenderable)
                    {
                        PolyStats& stats = mLodPolyStats[chunk->mLodLevel];
                        stats.sum_xi_minus_mean_sq += std::pow(chunk->mListTrianglesVertexCount - stats.mean, 2);
                    }
                    
                    for(OctreeChunk* child : chunk->mChildren)
                    {
                        if(child)
                        {
                            stats_sum_xi_minus_mean_sq(child);
                        }
                    }
                };

                it = chunkMgr->getChunkIterator();
                while(it.hasMoreElements())
                {
                    VT::OctreeChunk* chunk = it.getNext();
                    stats_sum_xi_minus_mean_sq(chunk);
                }
                
                for(PolyStats& stats : mLodPolyStats)
                {
                    stats.sigma = std::sqrt( stats.sum_xi_minus_mean_sq / stats.n );
                    //std::cout << "mean " << stats.mean << " sigma " << stats.sigma << std::endl;
                }
                
                
                /*
                // poly count > mean + sigma increase lod level
                // poly count < mean - sigma 'could' decrease lod level
                std::function< void (VT::OctreeChunk* chunk) > tweek_lod;
                tweek_lod = [&chunkMgr,
                             &tweek_lod,
                             &mLodPolyStats = this->mLodPolyStats](VT::OctreeChunk* chunk)
                {
                    if(chunk->mRenderable && chunk->mRenderable->isVisible())
                    {
                        PolyStats& stats = mLodPolyStats[chunk->mLodLevel];
                        if(chunk->mListTrianglesVertexCount > stats.mean + stats.sigma &&
                           chunk->mLodLevel + 1 < mLodPolyStats.size() )
                        {
                            chunk->setRenderableVisible(false);
                            for(int i=0; i<6; i++)
                            {
                                if(chunk->mTransition[i])
                                {
                                    chunk->mTransition[i]->setVisible(false);
                                }
                            }
                            
                            for(int childIdx=0; childIdx<8; childIdx++)
                            {
                                if(chunk->mTimeStamp < chunk->getDensity()->mTimeStamp && !chunk->mChildren[childIdx])
                                {
                                    chunkMgr->create(chunk->mIndex*2 + chunkMgr->getQuadrant(childIdx), chunk->mLodLevel+1, chunk, false);
                                }
                                
                                VT::OctreeChunk* child = chunk->mChildren[childIdx];
                                if(child)
                                {
                                    if(child->mTimeStamp < child->getDensity()->mTimeStamp)
                                    {
                                        chunkMgr->recreate(child);
                                    }
                                    child->setRenderableVisible(true);
                                }
                            }
                        }
                    }
                    for(OctreeChunk* child : chunk->mChildren)
                    {
                        if(child)
                        {
                            tweek_lod(child);
                        }
                    }
                };

                it = chunkMgr->getChunkIterator();
                while(it.hasMoreElements())
                {
                    VT::OctreeChunk* chunk = it.getNext();
                    tweek_lod(chunk);
                }
                
                
                
                std::function< void (VT::OctreeChunk* chunk) > zero_transitions;
                zero_transitions = [&zero_transitions](VT::OctreeChunk* chunk)
                {
                    for( ProceduralRenderable* transition : chunk->mTransition )
                    {
                        if(transition)
                        {
                            transition->setVisible(false);
                        }
                    }
                    for(OctreeChunk* child : chunk->mChildren)
                    {
                        if(child)
                        {
                            zero_transitions(child);
                        }
                    }
                };
                it = chunkMgr->getChunkIterator();
                while(it.hasMoreElements())
                {
                    VT::OctreeChunk* chunk = it.getNext();
                    zero_transitions(chunk);
                }
                */
            }
        }
        
        
        OctreeChunkSelector::OctreeChunkSelector() : mSelected(0)
        {
        }
        
        OctreeChunkSelector::~OctreeChunkSelector()
        {
        }
        
        void OctreeChunkSelector::select( VT::OctreeChunk* chunk )
        {
            std::function< void ( VT::OctreeChunk* , bool ) > set_visible_recursive;
            
            set_visible_recursive = [ &set_visible_recursive ]( VT::OctreeChunk* chunk, bool visible )
            {
                if(chunk->getWireBoundingBox())
                    chunk->getWireBoundingBox()->setVisible(visible);
                
                for( VT::OctreeChunk* child : chunk->mChildren )
                {
                    if(child)
                    {
                        set_visible_recursive(child, visible);
                    }
                }
            };
            
            if(chunk != mSelected)
            {
                if(mSelected)
                {
                    set_visible_recursive( mSelected, false );
                }

                mSelected = chunk;

                if(mSelected)
                {
                    set_visible_recursive( mSelected, true );
                }
            }
        }
    }
}
