#include "GOOFStopwatch.h"
#include <OgreRoot.h>

namespace GOOF
{
    Stopwatch::Stopwatch()
    {
        reset();
    }
    
    void Stopwatch::start()
    {
        mMark = Ogre::Root::getSingleton().getTimer()->getMilliseconds();
    }
    
    void Stopwatch::stop()
    {
        mCount++;
        mLast = Ogre::Root::getSingleton().getTimer()->getMilliseconds() - mMark;
        mAccum += mLast;
        mBest = std::min(mBest, mLast);
        mWorst = std::max(mWorst, mLast);
        mAverage = (float)mAccum/ (float)mCount;
    }
    
    void Stopwatch::reset()
    {
        mCount = 0;
        mAccum = 0;
        mBest = std::numeric_limits<Ogre::uint64>::max();
        mWorst = 0;
        mAverage = 0.0;
    }
    
    void Stopwatch::restart()
    {
        reset();
        start();
    }
    
    Ogre::Real Stopwatch::getAverageTime() const
    {
        return (Ogre::Real) mAccum/ (Ogre::Real) mCount;
    }
    
    void Stopwatch::split(const char* format, ...)
    {
        stop();
        {
            va_list ap;
            va_start(ap, format);
            vprintf(format, ap);
            va_end(ap);
            printf("time %fs\n", (float)mLast/1e03);
        }
        start();
    }
    
    void Stopwatch::toStringVector( Ogre::StringVector& parts )
    {
        parts.resize(5);
        parts[0] = Ogre::StringConverter::toString(mAverage, 3);
        parts[1] = Ogre::StringConverter::toString(mCount);
        parts[2] = Ogre::StringConverter::toString((long) mAccum);
        parts[3] = Ogre::StringConverter::toString((long) mBest);
        parts[4] = Ogre::StringConverter::toString((long) mWorst);
    }
    
    void Stopwatch::catToString( Ogre::String& str ) const
    {
        str +=
        Ogre::StringConverter::toString(mAverage, 3, 11, ' ') + " av " +
        Ogre::StringConverter::toString(mCount, 11, ' ') + " count " +
        Ogre::StringConverter::toString((long) mAccum, 11, ' ') + " accum " +
        Ogre::StringConverter::toString((long) mBest, 8, ' ') + " best " +
        Ogre::StringConverter::toString((long) mWorst, 8, ' ') +" worst";
    }
    
    void Stopwatch::log( const Ogre::String& text )
    {
        Ogre::String log = text + " ";
        catToString( log );
        Ogre::LogManager::getSingleton().logMessage( log );
    }
}
