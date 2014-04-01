#include "radiant.h"

using namespace ::radiant;

void radiant::HandleAssert(const char* assertMsg)
{
   radiant::log::Flush();
   if (IsDebuggerPresent()) {
      DebugBreak();
   } else {
      ::MessageBox(NULL, assertMsg, "Stonehearth Assertion Failed", MB_OK | MB_ICONEXCLAMATION);
#ifndef _DEBUG
      throw new CrashException();
#endif
   }
}