#include "radiant.h"
#include "metrics.h"
#include <iomanip>
#include <algorithm>

using namespace radiant;
using namespace radiant::metrics;

std::map<platform::ThreadId, std::unique_ptr<store>> radiant::metrics::store::_thread_stores;

store &store::get_current_thread_store()
{
   platform::ThreadId id = platform::GetCurrentThreadId();
   auto i = _thread_stores.find(id);
   if (i != _thread_stores.end()) {
      return *i->second;
   }
   store *s = new store();
   _thread_stores[id] = std::unique_ptr<store>(s);
   return *s;
}

store::store()
{
   reset();
}

void store::push_category(std::string const& category)
{
   _categories.push(_categories.top()->get_child(category));
}

void store::pop_category(metrics::time delta)
{
   profile_entry *pe = _categories.top();
   pe->duration += delta;
   _categories.pop();
}

void store::dump_stats()
{
   LOG(core.perf, 1) << "----------------------------";
   _root_category->dump_stats("", 1, 1);
}

void store::reset()
{
   while (_categories.size()) {
      _categories.pop();
   }
   _root_category = std::unique_ptr<profile_entry>(new profile_entry());
   _categories.push(_root_category.get());
}

store::profile_entry *store::profile_entry::get_child(std::string const& subcat) {
   for (unsigned int i = 0; i < children.size(); i++) {
      if (children[i]->category == subcat) {
         return children[i].get();
      }
   }
   profile_entry *child = new profile_entry(subcat);
   children.push_back(std::unique_ptr<profile_entry>(child));
   return child;
}

void store::profile_entry::dump_stats(std::string const& parent_category, metrics::time parent_duration, metrics::time overall_duration)
{
   overall_duration = std::max(overall_duration, duration);
   parent_duration = std::max(parent_duration, duration);

   metrics::time children_duration = 0;
   std::for_each(begin(children), end(children), [&](const std::unique_ptr<profile_entry> &child) {
      children_duration += child->duration;
   });
   metrics::time unbilled_duration = duration - children_duration;
   float percent_overall = 100 * ((float)unbilled_duration) / overall_duration;
   float percent_of_parent = 100 * ((float)duration / parent_duration);

   std::string full_category;
   if (category.size()) {
      // full_category = (parent_category.size() ? (parent_category + "." + category) : (category));
      full_category = parent_category + std::string("   ");
      LOG(core.perf, 1) << std::setfill(' ') << std::left << std::setw(85) << (full_category + category) << std::setw(10) << std::setprecision(2) << std::fixed << percent_overall << "% (" << percent_of_parent << "% of parent)";
   }

   std::for_each(begin(children), end(children), [&](const std::unique_ptr<profile_entry> &child) {
      child->dump_stats(full_category, duration, overall_duration);
   });

   duration = 0;
}
