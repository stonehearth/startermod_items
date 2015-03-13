#include "pch.h"
#include "low_mem_allocator.h"
#include "core/config.h"
#include <iomanip>
#include <boost/interprocess/creation_tags.hpp>

using namespace ::radiant;
using namespace ::radiant::lua;

static const int MEMORY_REPORT_TIMEOUT = 60 * 1000;

/* Number of top bits of the lower 32 bits of an address that must be zero.
 * Apparently 0 gives us full 64 bit addresses and 1 gives us the lower 2GB.
 */
#define NTAVM_ZEROBITS		1

typedef long (*PNTAVM)(HANDLE handle, void **addr, ULONG zbits,
		       size_t *size, ULONG alloctype, ULONG prot);

DEFINE_SINGLETON(LowMemAllocator);

boost::detail::spinlock __lock;

LowMemAllocator::LowMemAllocator() :
   _allocCount(0),
   _byteCount(0),
   _freeCount(0),
   _warnedHeapFull(false)
{
   _reportTimer.set(0);

   int mb = core::Config::GetInstance().Get<int>("luajit_lowmem_size", 512); 
   size_t requestedSize = mb * 1024 * 1024;
   _heapSize = requestedSize;

   PNTAVM ntavm = (PNTAVM)GetProcAddress(GetModuleHandleA("ntdll.dll"),
				                             "NtAllocateVirtualMemory");

  DWORD olderr = GetLastError();
  _allocatorMemory = nullptr;
  long st = ntavm(INVALID_HANDLE_VALUE, &_allocatorMemory, NTAVM_ZEROBITS, &_heapSize,
		            MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
  if (st != 0) {
     LOG(lua.code, 0) << "failed to allocate low memory for luajit interpreter: " << std::hex << st;
  }
  if (_heapSize != requestedSize) {
     LOG(lua.code, 0) << "OS would only return " << FormatSize(_heapSize) << " for the shared lua heap (wanted: " << FormatSize(requestedSize) << ")";
  }
  SetLastError(olderr);
   _allocator = Allocator(boost::interprocess::create_only_t(), _allocatorMemory, _heapSize);
}

LowMemAllocator::~LowMemAllocator()
{
   free(_allocatorMemory);
}

std::string LowMemAllocator::FormatSize(int size) const
{
   static std::string suffixes[] = { "B", "KB", "MB", "GB", "TB", "PB" /* lol! */ };
   std::string* suffix = suffixes;
   float total = static_cast<float>(size);
   while (total > 1024.0) {
      total /= 1024.0;
      suffix++;
   }
   return BUILD_STRING(std::fixed << std::setw(5) << std::setprecision(3) << total << " " << *suffix);
};

void *LowMemAllocator::allocate(size_t size)
{
   std::lock_guard<boost::detail::spinlock> lock(__lock);

   if (_reportTimer.expired()) {
      LOG(lua.code, 1) << "lua shared heap size: " << FormatSize(_byteCount) << " (live allocs:" << (_allocCount-_freeCount) << ")";
      _reportTimer.set(MEMORY_REPORT_TIMEOUT);
   }
   if (!_warnedHeapFull && _byteCount > _heapSize * 0.9) {
      LOG(lua.code, 0) << "lua shared heap is nearly full!";
      _warnedHeapFull = true;
   }
   
   _byteCount += size;
   _allocCount++;

   void *ptr = nullptr;
   auto i = _freelist.find(size);
   if (i != _freelist.end()) {
      std::vector<void*>& alloc = i->second;
      ptr = alloc.back();
      alloc.pop_back();
      if (alloc.empty()) {
         _freelist.erase(i);
      }
   } else {
      ptr = _allocator.allocate(size);
   }
   _allocSizes[ptr] = size;

   return ptr;
}

void LowMemAllocator::deallocate(void *ptr)
{
   std::lock_guard<boost::detail::spinlock> lock(__lock);

   if (!ptr) {
      return;
   }

   auto i = _allocSizes.find(ptr);
   ASSERT(i != _allocSizes.end());

   int size = i->second;
   _allocSizes.erase(i);

   _byteCount -= size;
   _freeCount++;

   _freelist[size].push_back(ptr);
   //return _allocator.deallocate(ptr);
}
