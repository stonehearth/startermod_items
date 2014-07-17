#include "radiant.h"
#include "store.h"
#include "map.h"
#include "map_iterator.h"

using namespace radiant;
using namespace radiant::dm;


template <typename M>
MapIterator<M>::MapIterator(M const& map, typename M::ContainerType::const_iterator i) :
   _map(map),
   _capturedTime(_map.GetLastModified())
{
   SetCurrentIterator(i);
}

template <typename M>
bool MapIterator<M>::operator==(MapIterator const& other) const
{
   RefreshIterator();
   other.RefreshIterator();

   if (_isEnd == other._isEnd) {
      return true;
   }
   return _currentIterator == other._currentIterator;
}

template <typename M>
bool MapIterator<M>::operator!=(MapIterator const& other) const
{
   return !(*this == other);
}

template <typename M>
typename MapIterator<M>::EntryType MapIterator<M>::operator*() const  
{
   RefreshIterator();
   ASSERT(!_isEnd);

   return *_currentIterator;
}

template <typename M>
typename MapIterator<M>::EntryType const* MapIterator<M>::operator->() const  
{
   RefreshIterator();
   ASSERT(!_isEnd);

   return &(*_currentIterator);
}

template <typename M>
MapIterator<M>& MapIterator<M>::operator++() 
{
   ASSERT(!_isEnd);
   RefreshIterator();

   if (!_isEnd) {
      M::ContainerType::const_iterator i = _currentIterator;
      ++i;
      SetCurrentIterator(i);
   }
   return *this;
}

template <typename M>
bool MapIterator<M>::IsOutOfDate() const
{
   return !_isEnd && (_map.GetLastModified() != _capturedTime);
}

template <typename M>
void MapIterator<M>::RefreshIterator() const
{
   if (IsOutOfDate()) {
      typename M::ContainerType const& container = _map.GetContainer();
      auto end = container.end();

      if (_currentKey == _map.GetLastErasedKey()) {
         _currentIterator = _map.GetLastErasedIterator();
      } else {
         _currentIterator = end;
      }

      _isEnd = _currentIterator == end;
      _capturedTime = _map.GetLastModified();
      if (!_isEnd) {
         _currentKey = _currentIterator->first;
      }
   }
}

template <typename M>
void MapIterator<M>::SetCurrentIterator(typename M::ContainerType::const_iterator i) const
{
   ASSERT(!IsOutOfDate());

   typename M::ContainerType const& container = _map.GetContainer();
   M::ContainerType::const_iterator const end = container.end();

   _isEnd = (i == end);
   _currentIterator = i;

   if (!_isEnd) {
      _currentKey = i->first;
      ++i;
   }
}

#define CREATE_MAP(M)    template MapIterator<M>;
#include "types/all_map_types.h"
#include "types/all_loader_types.h"
ALL_DM_MAP_TYPES
