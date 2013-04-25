#include "radiant.h"
#include "metrics.h"

using namespace radiant;
using namespace radiant::metrics::profile;

counter::counter(metrics::store &store, std::string &category) :
   _store(store)
{
   _store.push_category(category);
   metrics::get_time(_start);
}

counter::counter(metrics::store &store, const char *category) :
   _store(store)
{
   _store.push_category(std::string(category));
   metrics::get_time(_start);
}

counter::~counter()
{
   metrics::time finish;
   metrics::get_time(finish);
   _store.pop_category(finish - _start);
}
