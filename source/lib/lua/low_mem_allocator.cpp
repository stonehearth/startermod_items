#include "pch.h"
#include "low_mem_allocator.h"
#include "core/config.h"
#include <iomanip>
#include <boost/interprocess/creation_tags.hpp>

using namespace ::radiant;
using namespace ::radiant::lua;

/* Number of top bits of the lower 32 bits of an address that must be zero.
 * Apparently 0 gives us full 64 bit addresses and 1 gives us the lower 2GB.
 */
#define NTAVM_ZEROBITS		1

typedef long (*PNTAVM)(HANDLE handle, void **addr, ULONG zbits,
		       size_t *size, ULONG alloctype, ULONG prot);

DEFINE_SINGLETON(LowMemAllocator);

LowMemAllocator::LowMemAllocator()
{
   int mb = core::Config::GetInstance().Get<int>("luajit_lowmem_size", 512);

   size_t size = mb * 1024 * 1024;
   PNTAVM ntavm = (PNTAVM)GetProcAddress(GetModuleHandleA("ntdll.dll"),
				                             "NtAllocateVirtualMemory");

  DWORD olderr = GetLastError();
  _allocatorMemory = nullptr;
  long st = ntavm(INVALID_HANDLE_VALUE, &_allocatorMemory, NTAVM_ZEROBITS, &size,
		            MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
  if (st != 0) {
     LOG(lua.code, 0) << "failed to allocate low memory for luajit interpreter: " << std::hex << st;
  }
  SetLastError(olderr);
   _allocator = Allocator(boost::interprocess::create_only_t(), _allocatorMemory, size);
}

LowMemAllocator::~LowMemAllocator()
{
   free(_allocatorMemory);
}

void *LowMemAllocator::allocate(size_t size)
{
   return _allocator.allocate(size);
}

void LowMemAllocator::deallocate(void *ptr)
{
   return _allocator.deallocate(ptr);
}
