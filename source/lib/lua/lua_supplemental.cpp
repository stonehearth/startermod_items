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
      return NULL;
   }
   std::streampos pos = lf->is->tellg();
   lf->is->seekg(0, std::ios::end);
   size_t count = (unsigned int)lf->is->tellg();
   lf->buf.resize(count);
   lf->is->seekg(pos, std::ios::beg);
   lf->is->read(&lf->buf[0], count);
   lf->is = nullptr;

   *size = lf->buf.size();
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
   lf.is->read(&c, 1);
   if (c == '#') {  /* Unix exec. file? */
      lf.extraline = 1;
      while (lf.is->good()) {
         lf.is->read(&c, 1);
         if (c == '\n') {
            lf.is->read(&c, 1);
            break;
         }
      }
   }
   if (c == LUA_SIGNATURE[0] && filename) {  /* binary file? */
      /* skip eventual `#!...' */
      while (lf.is->good()) {
         lf.is->read(&c, 1);
         if (c == LUA_SIGNATURE[0]) {
            break;
         }
      }
      lf.extraline = 0;
   }
   lf.is->putback(c);
   status = lua_load(L, getF, &lf, lua_tostring(L, -1));
   lua_remove(L, fnameindex);
   return status;
}
