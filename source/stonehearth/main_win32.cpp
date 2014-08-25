#if defined(ENABLE_MEMPRO)
#define OVERRIDE_NEW_DELETE
#include "C:\\Program Files\\PureDevSoftware\\MemPro\\MemProLib\\src_combined\MemPro.cpp"
#endif

#include <signal.h>
#include <boost/filesystem.hpp>
#include "radiant.h"


/*
 * EASTL expects these operators to exist.  Just redirect them to the standard operator new[]
 */
void* operator new[](size_t size, const char* pName, int flags, unsigned debugFlags, const char* file, int line)
{
   return new char[size];
}

void* operator new[](size_t size, size_t alignment, size_t alignmentOffset, const char* pName, int flags, unsigned debugFlags, const char* file, int line)
{
   return new char[size];
}

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
