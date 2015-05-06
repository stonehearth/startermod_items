#ifndef _RADIANT_FILE_H
#define _RADIANT_FILE_H

#include <istream>

namespace radiant {
   class io {
   public:
      static std::string read_contents(std::istream& in) {
         std::string contents;

         in.seekg(0, std::ios::end);
         std::streampos count = in.tellg();
         if (count > 0) {
            contents.resize((uint)count);
            in.seekg(0, std::ios::beg);
            in.read(&contents[0], count);
         
            unsigned int read_count = (unsigned int)in.gcount();
            if (read_count != count) {
               contents.resize(read_count);
            }
         }

         return contents;
      }

      static std::string clip_middle_of_logfile(std::ifstream &in, int maxSize) {
         static std::string filler("\n\n... clipped ...\n\n");

         int halfSize = maxSize / 2;

         in.seekg(0, std::ios::end);
         std::streampos count = in.tellg();
         in.seekg(0, std::ios::beg);

         if (count <= maxSize) {
            return read_contents(in);
         }
         std::string clipped;

         int size = maxSize + filler.size() + 1;
         clipped.resize(size);
         char* p = &clipped[0];
         in.read(p, halfSize);

         p += halfSize;
         strcpy(p, filler.c_str());
         p += filler.size();

         in.seekg(-halfSize, std::ios::end);
         in.read(p, halfSize);
         p += halfSize;

         size = p - &clipped[0];
         clipped.resize(size);

         return clipped;
      }
   };
};

#endif // _RADIANT_JSON_H
