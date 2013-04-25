#include "radiant.h"
#include "backtrace.h"
#include "StackWalker.h"
#include <dbghelp.h>

class MyStackWalker : public StackWalker
{
public:
  MyStackWalker() : StackWalker() {}
  MyStackWalker(DWORD dwProcessId, HANDLE hProcess) : StackWalker(dwProcessId, hProcess) {}
  virtual void OnOutput(LPCSTR szText) {
     LOG(ERROR) << "BACKTRACE: " << szText;
     StackWalker::OnOutput(szText);
  }
};

LONG WINAPI ::radiant::platform::PrintBacktrace(EXCEPTION_POINTERS* pExp, DWORD dwExpCode)
{
  MyStackWalker sw;

  sw.ShowCallstack(GetCurrentThread(), pExp->ContextRecord);
  return EXCEPTION_EXECUTE_HANDLER;
}
