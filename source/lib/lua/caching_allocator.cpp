#include "pch.h"
#include "caching_allocator.h"
#include "core/config.h"
#include "core/system.h"
#include <iomanip>
#include <tbb/spin_mutex.h>
#include <boost/interprocess/creation_tags.hpp>

using namespace ::radiant;
using namespace ::radiant::lua;

// The log level required for lua.memory to actually dump the memory
// reports.
static const int VerboseReportsLogLevel = 1;

/* Number of top bits of the lower 32 bits of an address that must be zero.
 * Apparently 0 gives us full 64 bit addresses and 1 gives us the lower 2GB.
 */
#define NTAVM_ZEROBITS		1

typedef long (*NtAllocateVirtualMemoryFn)(HANDLE handle, void **addr, ULONG zbits,
		       size_t *size, ULONG alloctype, ULONG prot);
typedef long (*NtFreeVirtualMemoryFn)(HANDLE ProcessHandle, void **BaseAddress, size_t* RegionSize, ULONG FreeType);

DEFINE_SINGLETON(CachingAllocator);

tbb::spin_mutex __lock;

CachingAllocator::CachingAllocator() :
   _byteCount(0),
   _lowMemoryHeapSize(0),
   _freelistByteCount(0),
   _state(UnInitialized),
   _warnedHeapFull(false),
   _lowMemoryHeap(nullptr)
{
}

void CachingAllocator::Start(bool useLowMemory)
{
   if (_state == UnInitialized) {
      if (useLowMemory) {
         InitializeLowMemoryAllocator();
      }
   }
}

void CachingAllocator::InitializeLowMemoryAllocator()
{
   ASSERT(_lowMemoryHeap == nullptr);
   ASSERT(_state == UnInitialized);
   ASSERT(core::System::IsProcess64Bit());

   // We only use the CachingAllocator on 64-bit systems, so allocating a lot of address
   // space here shouldn't be a problem, unless we start hitting the commit limits of
   // the operating system (UG!).  If that happens, I expect we'll hit the 
   // actualSize != requestedSize case, below, a lot.
   int mb = core::Config::GetInstance().Get<int>("luajit_lowmem_size", 768); 
   size_t requestedSize = mb * 1024 * 1024;

   HMODULE ntdll = GetModuleHandleA("ntdll.dll");
   if (!ntdll) {
      LOG(lua.memory, 0) << "could not find ntdll.dll.";
      _state = CannotStart;
      return;
   }

   NtAllocateVirtualMemoryFn alloc = (NtAllocateVirtualMemoryFn)GetProcAddress(ntdll, "NtAllocateVirtualMemory");
   if (!alloc) {
      LOG(lua.memory, 0) << "failed to find NtAllocateVirtualMemory.";
      _state = CannotStart;
      return;
   }

   DWORD olderr = GetLastError();
   size_t actualSize = requestedSize;
   long st = alloc(INVALID_HANDLE_VALUE, &_lowMemoryHeap, NTAVM_ZEROBITS, &actualSize,
                   MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
   if (st != 0) {
      LOG(lua.memory, 0) << "failed to allocate low memory for luajit interpreter: " << std::hex << st;
      _state = CannotStart;
      _lowMemoryHeap = nullptr;
      return;
   }

   if (actualSize != requestedSize) {
      LOG(lua.memory, 0) << "OS would only return " << FormatSize(_lowMemoryHeapSize) << " for the shared lua heap (wanted: " << FormatSize(requestedSize) << ")";
      NtFreeVirtualMemoryFn dealloc = (NtFreeVirtualMemoryFn)GetProcAddress(ntdll, "NtFreeVirtualMemory");
      if (dealloc) {
         dealloc(INVALID_HANDLE_VALUE, &_lowMemoryHeap, &_lowMemoryHeapSize, MEM_RELEASE | MEM_DECOMMIT);
      }
      _state = CannotStart;
      _lowMemoryHeap = nullptr;
      return;
   }

   SetLastError(olderr);
   _lowMemoryHeapSize = actualSize;
   _lowMemoryAllocator = Allocator(boost::interprocess::create_only_t(), _lowMemoryHeap, _lowMemoryHeapSize);
   _state = Started;
}


void* CachingAllocator::LuaAllocFn(void *, void *ptr, size_t osize, size_t nsize)
{
   return CachingAllocator::GetInstance().LuaAlloc(ptr, osize, nsize);
}

void* CachingAllocator::LuaAlloc(void *ptr, size_t osize, size_t nsize)
{
   void *realloced;

   // according to the standard, there is no type which can contain the correct signed
   // value of a size_t if the result is negative.  So incur the branch penalty!
   if (nsize > osize) {
      _byteCount += nsize - osize;
   } else {
      _byteCount -= osize - nsize;
   }

   if (nsize == 0) {
      Deallocate(ptr, osize);
      realloced = nullptr;
   } else {
      realloced = Allocate(nsize);
      if (osize) {
         memcpy(realloced, ptr, std::min(nsize, osize));
         Deallocate(ptr, osize);
      }
   }
   return realloced;
}

void CachingAllocator::ReportMemoryStats(bool force)
{
   tbb::spin_mutex::scoped_lock lock(__lock);
   ReportMemoryStatsUnlocked(force);
}

void CachingAllocator::ReportMemoryStatsUnlocked(bool force)
{
   if (_state != Started) {
      return;
   }

   LOG(lua.memory, 1) << "lua shared heap size: " << FormatSize(_byteCount) << ".  "
                      << "freelist overhead: " << FormatSize(_freelistByteCount)
                      << "  (" << std::setprecision(2) << (100.0 * _freelistByteCount/ _byteCount) << "%).  "
                      << "total bytes allocated: " << FormatSize(_byteCount + _freelistByteCount) << ".";

   uint level = force ? 1 : VerboseReportsLogLevel;

   if (LOG_IS_ENABLED(lua.memory, level)) {
      size_t maxAllocCount = 0;
      size_t totalAllocCount = 0;
      int threshold = 5;

      // Create an array of (allocSize, allocCount) tuples for all the memory we've allocated.
      typedef std::pair<size_t, size_t> SizeCount;
      std::vector<SizeCount> sizes;
      for (auto const &entry : _allocCounts) {
         size_t size = entry.first;
         size_t allocCount = entry.second;
         totalAllocCount += allocCount;
         maxAllocCount = std::max(maxAllocCount, allocCount);
         sizes.emplace_back(size, allocCount);
      }
      
      // Sort by total size 
      std::sort(sizes.begin(), sizes.end(), [](SizeCount const& lhs, SizeCount const& rhs) -> bool {
         return lhs.second > rhs.second;
      });

      // Write the table.
      LOG(lua.memory, level) << "o: outstanding allocs, fl: free list len";
      for (auto const &st : sizes) {
         size_t allocSize = st.first;
         size_t allocCount = st.second;

         size_t freeListLen = allocSize < CACHE_MEMORY_SIZE ? (_freelist[allocSize].size()) : 0;
         size_t rows = allocCount * 40 / maxAllocCount;
         ASSERT(rows >= 0 && rows <= 40);
         std::string stars(rows, '#');

         LOG(lua.memory, level) << std::setw(8)  << allocSize << " bytes "
                            << "(o:" << std::setw(7) << allocCount
                            << " fl:" << std::setw(7)  << freeListLen
                            << "  " << std::setw(7)  << std::setprecision(2) << std::fixed << (100.0f * freeListLen / allocCount) << "%"
                            << ") : "
                            << stars;

         if (allocCount < totalAllocCount * threshold / 100) {
            LOG(lua.memory, level) << "    ... remaining allocs are each < " << threshold << "% of the count";
            break;
         }
      }
   }
}

CachingAllocator::~CachingAllocator()
{
   free(_lowMemoryHeap);
}

std::string CachingAllocator::FormatSize(size_t size) const
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

void *CachingAllocator::Allocate(size_t size)
{
   tbb::spin_mutex::scoped_lock lock(__lock);

   if (IsUsingLowMemory() && !_warnedHeapFull && _byteCount > _lowMemoryHeapSize * 0.9) {
      LOG(lua.memory, 0) << "lua shared heap is nearly full!";
      ReportMemoryStats();
      _warnedHeapFull = true;
   }
   
   ++_allocCounts[size];

   // if the requested size is less than the limit, look for one on the free list
   void *ptr = nullptr;
   if (UseFreeList(size)) {
      std::vector<void*>& alloc = _freelist[size];
      if (!alloc.empty()) {
         ptr = alloc.back();
         alloc.pop_back();
         _freelistByteCount -= size;
      }
   }

   // either we shouldn't be using the free list or it's empty.  either way, alloc.
   if (!ptr) {
      if (IsUsingLowMemory()) {
         try {
            ptr = _lowMemoryAllocator.allocate(size);
         } catch (boost::interprocess::bad_alloc const& e) {
            // we ran out of memory.  this means either there's a *serious* leak in lua or we somehow allocated
            // too much.  Look at the memory report in the log for this crash to see which
            std::string msg = BUILD_STRING("error in lua allocator attempting to allocate " << size  << " bytes (" << e.what() << ")");
            LOG(lua.memory, 0) << msg;
            ReportMemoryStatsUnlocked(true);

            // Now report to the user that we're going to down, why, then crash so we get a report on the
            // server.
            ::MessageBox(NULL, msg.c_str(), "Stonehearth Assertion Failed", MB_OK | MB_ICONEXCLAMATION);
            *(int *)0 = 0;    // crash harder!!!
         }
      } else {
         if (size > 50000000) {
            std::string msg = BUILD_STRING("Allocating " << size  << " bytes for lua.");
            LOG(lua.memory, 0) << msg;
         }
         ptr = new char[size];
      }
   }

   return ptr;
}

void CachingAllocator::Deallocate(void *ptr, size_t size)
{
   tbb::spin_mutex::scoped_lock lock(__lock);

   if (!ptr) {
      return;
   }

   --_allocCounts[size];

   // if small enough, stick on the freelist.  Otherwise, nuke it
   if (UseFreeList(size)) {
      _freelist[size].emplace_back(ptr);
      _freelistByteCount += size;
   } else {
      if (IsUsingLowMemory()) {
         _lowMemoryAllocator.deallocate(ptr);
      } else {
         delete [] ptr;
      }
   }
}
