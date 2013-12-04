//#define OVERRIDE_NEW_DELETE
//#include "MemProLib/src_combined/MemPro.cpp"

#include <signal.h>
#include <boost/filesystem.hpp>
#include "radiant.h"

extern int lua_main(int argc, const char **argv);

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
#ifdef _MSC_VER
	// fix for https://svn.boost.org/trac/boost/ticket/6320
   std::locale default("");
   boost::filesystem::path::imbue(default);
#endif 
   return lua_main(__argc, (const char **)__argv);
}
