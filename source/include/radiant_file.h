#ifndef _RADIANT_FILE_H
#define _RADIANT_FILE_H

#include <istream>

namespace radiant {
   class io {
   public:
      static std::string read_contents(std::istream& in) {
         std::string contents;

         in.seekg(0, std::ios::end);
         unsigned int count = (unsigned int)in.tellg();
         contents.resize(count);
         in.seekg(0, std::ios::beg);
         in.read(&contents[0], count);
         
         unsigned int read_count = (unsigned int)in.gcount();
         if (read_count != count) {
            contents.resize(read_count);
         }

         return contents;
      }
   };
};

#endif // _RADIANT_JSON_H
