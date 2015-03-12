#ifndef _RADIANT_CORE_DOUBLE_BUFFER_H
#define _RADIANT_CORE_DOUBLE_BUFFER_H

#include <mutex>
#include "namespace.h"

BEGIN_RADIANT_CORE_NAMESPACE

/*
 * -- DoubleBuffer<T> class
 *
 * Used to double buffer global objects where a producer writes to one while
 * a consumer reads from the last copy
 *
 */

template <typename T>
class DoubleBuffer
{
public:
   DoubleBuffer() :
      _back(1),
      _front(0)
   {
   }

   T& GetBackBuffer() {
      std::lock_guard<std::mutex> guard(_lock);
      return _buffers[_back];
   }

   T& LockFrontBuffer() {
      _lock.lock();
      return _buffers[_front];
   }

   void UnlockFrontBuffer() {
      _lock.unlock();
   }

   void Swap() {
      std::lock_guard<std::mutex> guard(_lock);
      std::swap(_back, _front);
   }

private:
   int         _back;
   int         _front;
   std::mutex  _lock;
   T           _buffers[2];
};

END_RADIANT_CORE_NAMESPACE

#endif // _RADIANT_CORE_DOUBLE_BUFFER_H
