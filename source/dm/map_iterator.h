#ifndef _RADIANT_DM_MAP_ITERATOR_H_
#define _RADIANT_DM_MAP_ITERATOR_H_

#include "object.h"
#include <unordered_map>

BEGIN_RADIANT_DM_NAMESPACE

template <class M>
class MapIterator
{
public:
   MapIterator(M const& map, typename M::ContainerType::const_iterator);

   typedef typename M::ContainerType::value_type EntryType;

public:
   bool operator==(MapIterator const& other) const;
   bool operator!=(MapIterator const& other) const;
   EntryType operator*() const;
   EntryType const* operator->() const;
   MapIterator& operator++();

private:
   void RefreshIterator() const;
   void SetCurrentIterator(typename M::ContainerType::const_iterator i) const;
   bool IsOutOfDate() const;

private:
   M const&                                  _map;
   mutable bool                              _isEnd;
   mutable GenerationId                      _capturedTime;
   mutable typename M::Key                            _currentKey;
   mutable typename M::ContainerType::const_iterator  _currentIterator;
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_MAP_ITERATOR_H_
