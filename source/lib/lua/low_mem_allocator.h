#ifndef _RADIANT_LUA_LOW_MEM_ALLOCATOR_H
#define _RADIANT_LUA_LOW_MEM_ALLOCATOR_H

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

public:
   void *allocate(size_t size);
   void deallocate(void *ptr);

private:
   typedef boost::interprocess::basic_managed_external_buffer<
      char,
      boost::interprocess::rbtree_best_fit<boost::interprocess::mutex_family>,
      boost::interprocess::iunordered_set_index
   > Allocator;

private:
   std::string LowMemAllocator::FormatSize(int size) const;

private:
   void *                     _allocatorMemory;
   Allocator                  _allocator;

   int                        _allocCount;
   int                        _byteCount;
   int                        _freeCount;
   size_t                     _heapSize;
   bool                       _warnedHeapFull;
   platform::timer            _reportTimer;

   std::unordered_map<void*, int>               _allocSizes;
   std::unordered_map<int, std::vector<void*>>  _freelist;
};

END_RADIANT_LUA_NAMESPACE

#endif //  _RADIANT_LUA_LOW_MEM_ALLOCATOR_H