#ifndef _RADIANT_BITVECTOR_H
#define _RADIANT_BITVECTOR_H

#include <vector>

namespace radiant {
   class bitvector {
      public:
         bitvector(int size) : _size(size), _bits(size / sizeof(int)) { }
         bitvector(int size, bool initial) : _size(size), _bits(size / sizeof(int), initial ? -1 : 0) { }

         int size() const { return _size; }
         bool is_set(int bit) const { return (_bits[bit / sizeof(int)] & (1 << (bit % sizeof(int)))) != 0; }
         bool is_clear(int bit) const { return !is_set(bit); }

         void clear() { _bits.clear(); }
         void set_bit(int bit) { _bits[bit / sizeof(int)] |= (1 << (bit % sizeof(int))); }
         void clear_bit(int bit) { _bits[bit / sizeof(int)] &= ~(1 << (bit % sizeof(int))); }

      protected:
         int            _size;
         vector<int>    _bits;
   };
};

#endif // _RADIANT_BITVECTOR_H
