#ifndef _RADIANT_METRICS_STORE_H
#define _RADIANT_METRICS_STORE_H

#include "platform//thread.h"
#include <vector>
#include <map>
#include <stack>
#include <memory>

namespace radiant {
   namespace metrics {
      class store {
         public:
            store();

            void push_category(std::string const& category);
            void pop_category(metrics::time delta);

            void dump_stats();
            void reset();

            static store &get_current_thread_store();

         private:
            store(const store &);   // non-copyable

         protected:
            struct profile_entry {
               profile_entry() : duration(0) { }
               profile_entry(std::string const& c) : category(c), duration(0) { }

               std::string                              category;
               metrics::time                       duration;
               std::vector<std::unique_ptr<profile_entry>>   children;

               profile_entry *get_child(std::string const& subcat);
               void dump_stats(std::string const& parent_category, metrics::time parent_duration, metrics::time overall_duration);
            };            

         protected:
            static std::map<platform::ThreadId, std::unique_ptr<store>> _thread_stores;

            std::unique_ptr<profile_entry>  _root_category;
            std::stack<profile_entry *>     _categories;
      };
   };
};

#endif // _RADIANT_METRICS_STORE_H
