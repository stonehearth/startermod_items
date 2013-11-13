#include "pch.h"
#include "lua_supplemental.h"
#include "resources/res_manager.h"

using namespace ::radiant;
using namespace ::radiant::lua;

typedef struct LoadF {
   int extraline;
   std::shared_ptr<std::istream> is;
   std::string buf;
} LoadF;

static const char *getF (lua_State *L, void *ud, size_t *size) {
   LoadF *lf = (LoadF *)ud;
   (void)L;
   if (lf->extraline) {
      lf->extraline = 0;
      *size = 1;
      return "\n";
   }
   if (!lf->is) {
      return nullptr;
   }

   int const file_buffer_size = 4000;

   lf->buf.resize(file_buffer_size);
   lf->is->read(&lf->buf[0], file_buffer_size);

   int const bytes_read = (int) lf->is->gcount();
   if (bytes_read < file_buffer_size) {
      lf->is = nullptr;
   }

   *size = (int) bytes_read;
   return lf->buf.data();
}

int luaL_loadfile_from_resource(lua_State *L, const char *filename) {
   LoadF lf;
   int status;
   char c;
   int fnameindex = lua_gettop(L) + 1;  /* index of filename on the stack */
   lf.extraline = 0;
   lua_pushfstring(L, "@%s", filename);
   lf.is = res::ResourceManager2::GetInstance().OpenResource(filename);

   // read the first byte 
   lf.is->read(&c, 1);

   // if run as a unix script, skip first line
   if (c == '#') {
      lf.extraline = 1;
      while (lf.is->good()) {
         lf.is->read(&c, 1);
         if (c == '\n') {
            // read the first byte of the "real" section (so we can put it back later...)
            lf.is->read(&c, 1);
            break;
         }
      }
   }

   // look for binary file header
   if (c == LUA_SIGNATURE[0]) {
      lf.extraline = 0;
      // code deleted...
      // using same logic as text file
   }

   // put back the first byte of the part we need to read
   lf.is->putback(c);

   status = lua_load(L, getF, &lf, lua_tostring(L, -1));
   lua_remove(L, fnameindex);
   return status;
}
