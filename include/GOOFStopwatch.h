#ifndef __GOOFStopwatch_H
#define __GOOFStopwatch_H

namespace GOOF
{
    class Stopwatch
    {
    public:
        Ogre::String name;
        
        // Num times stopped
        Ogre::uint32 mCount;
        
        // Total time accumulated
        Ogre::uint64 mAccum;
        
        // Best split time
        Ogre::uint64 mBest;
        
        // Worest split time
        Ogre::uint64 mWorst;
        
        // Last split time
        Ogre::uint64 mLast;
        
        // Average split time
        float mAverage;
        
    private:
        // Point at which we started the stopwatch
        Ogre::uint64 mMark;
        
        Ogre::Real getAverageTime() const;
        
    public:
        Stopwatch();
        
        void start();
        void stop();
        void reset();
        
        // reset() then start().
        void restart();
        
        // [ mAverage, mCount, mAccum, mBest, mWorst ].
        void toStringVector( Ogre::StringVector& parts );
        
        // str += [ mAverage, mCount, mAccum, mBest, mWorst ].
        void catToString( Ogre::String& str ) const;
        
        // Output to Ogre's log [ text, mAverage, mCount, mAccum, mBest, mWorst ]
        void log( const Ogre::String& text );
        
        // stop(), logs the last time, start().
        void split( const char* format, ... );
    };
    

}
#endif
