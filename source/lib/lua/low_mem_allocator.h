#ifndef _RADIANT_LUA_LOW_MEM_ALLOCATOR_H
#define _RADIANT_LUA_LOW_MEM_ALLOCATOR_H

#include <array>
#include <boost/interprocess/managed_external_buffer.hpp>
#include <boost/interprocess/mem_algo/rbtree_best_fit.hpp>
#include <boost/interprocess/indexes/iunordered_set_index.hpp>

#include "core/singleton.h"
#include "lib/lua/lua.h"

BEGIN_RADIANT_LUA_NAMESPACE

class LowMemAllocator : public core::Singleton<LowMemAllocator> {
public:
   LowMemAllocator();
   ~LowMemAllocator();

   bool Start();
   void ReportMemoryStats(bool force = false);

   static void* LuaAllocFn(void *ud, void *ptr, size_t osize, size_t nsize);

private:
   typedef boost::interprocess::basic_managed_external_buffer<
      char,
      boost::interprocess::rbtree_best_fit<boost::interprocess::mutex_family>,
      boost::interprocess::iunordered_set_index
   > Allocator;

   enum {
      CACHE_MEMORY_SIZE = 128, // allocs below this size will be kept in a freelist.
   };

   enum State {
      UnInitialized,
      Started,
      CannotStart,
   };
private:
   bool InitializeAllocator();
   void* LuaAlloc(void *ptr, size_t osize, size_t nsize);
   std::string LowMemAllocator::FormatSize(size_t size) const;
   void *Allocate(size_t size);
   void ReportMemoryStatsUnlocked(bool force);
   void Deallocate(void *ptr, size_t size);
    
   inline bool UseFreeList(size_t size) {
      return size < CACHE_MEMORY_SIZE;
   }

private:
   void *                     _allocatorMemory;
   Allocator                  _allocator;

   State                      _state;
   size_t                     _byteCount;
   size_t                     _heapSize;
   bool                       _warnedHeapFull;
   platform::timer            _reportTimer;

   std::array<std::vector<void*>, CACHE_MEMORY_SIZE>  _freelist;
   size_t                                             _freelistByteCount;
   std::unordered_map<size_t, size_t>                 _allocCounts;
   std::unordered_map<size_t, size_t>                 _allocTotalSize;
};

END_RADIANT_LUA_NAMESPACE

#endif //  _RADIANT_LUA_LOW_MEM_ALLOCATOR_H