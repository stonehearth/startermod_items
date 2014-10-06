#include "radiant.h"

using namespace ::radiant;

void radiant::HandleAssert(std::string const& assertMsg)
{
   LOG(app, 0) << assertMsg;
   radiant::log::Flush();

   if (IsDebuggerPresent()) {
      DebugBreak();
   } else {
      ::MessageBox(NULL, assertMsg.c_str(), "Stonehearth Assertion Failed", MB_OK | MB_ICONEXCLAMATION);
#ifndef _DEBUG
      throw new CrashException();
#endif
   }
}