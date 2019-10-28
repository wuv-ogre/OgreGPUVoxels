#ifndef __GOOFNameGenerator_H__
#define __GOOFNameGenerator_H__

#include <string>

namespace GOOF
{
    /**
     Ogre::NameGenerator for a non map version
    */
    class NameGenerator
    {
    public:
        
        /**
         * Gets the next unique name.
         */
        static std::string Next();
        
        /**
         * Gets the next unique name for a given prefix.
         */
        static std::string Next(const std::string& prefix);
        
        /**
         * Counts the number of unique auto-generated names.
         */
        static size_t Count();
        
        /**
         * Counts the number of names generated for a given prefix.
         * @param prefix The prefix of the generated names.
        */
        static size_t Count(const std::string& prefix);
    private:
        NameGenerator(void);
        ~NameGenerator(void);
        
        typedef std::map<std::string, unsigned int> NameCountMap;
        static NameCountMap s_nameCount;
    };
}

#endif

