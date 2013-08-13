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

   class IResponse
   {
   public:
      virtual ~IResponse() { }
      virtual void SetResponse(int status) { SetResponse(status, "", ""); }
      virtual void SetResponse(int status, std::string const& response, std::string const& mimeType) = 0;
   };

   class MouseEvent {
      public:
         int         wheel;
         int         x;
         int         y;
         int         dx;
         int         dy;
         int         drag_start_x;
         int         drag_start_y;
         bool        dragging;
         bool        dragend;
         bool        up[16];
         bool        down[16];
         bool        buttons[16];
   };

   class KeyboardEvent {
      public:
         bool        down;
         int         key;
   };

   class RawInputEvent {
      public:
         UINT     msg;
         WPARAM   wp;
         LPARAM   lp;
   };

};

#endif // _RADIANT_JSON_H
