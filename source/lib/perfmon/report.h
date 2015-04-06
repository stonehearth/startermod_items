#ifndef _RADIANT_PERFMON_REPORT_H
#define _RADIANT_PERFMON_REPORT_H

#include "namespace.h"
#include "core/guard.h"

BEGIN_RADIANT_PERFMON_NAMESPACE

template <typename Container, int Width>
void ReportCounters(Container const& container, std::function<void(typename Container::key_type const&, typename Container::mapped_type const&, size_t)> cb)
{
   typename Container::mapped_type maxValue = 0;

   std::vector<std::pair<Container::key_type, Container::mapped_type>> order;
   for (Container::value_type const &entry : container) {
      maxValue = std::max(maxValue, entry.second);
      order.emplace_back(entry);
   }

   // Sort by total size 
   std::sort(order.begin(), order.end(),
      [](Container::value_type const& lhs, Container::value_type const& rhs) -> bool {
         return lhs.second > rhs.second;
      });

   // Write the table.
   for (auto const &o : order) {
      core::StaticString reason = o.first;
      perfmon::CounterValueType duration = o.second;

      size_t rows = maxValue ? (size_t)(duration * Width / maxValue) : 0;
      ASSERT(rows >= 0 && rows <= Width);
      cb(o.first, o.second, rows);
   }
}

END_RADIANT_PERFMON_NAMESPACE

#endif // _RADIANT_PERFMON_REPORT_H
