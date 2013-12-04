#ifndef _RADIANT_PLATFORM_UTILS_H
#define _RADIANT_PLATFORM_UTILS_H

namespace radiant {
   namespace platform {
      inline int get_current_time_in_ms() {
         WIN32_ONLY(return timeGetTime();)
      }

      inline void sleep(int ms) {
         WIN32_ONLY(::Sleep(ms);)
      }

      class timer {
         public:
            timer(int expire_time = 0) { set(expire_time); }
            
            void set(int expire_time) { _expire_time = get_current_time_in_ms() + expire_time; }

            void advance(int more_time) { _expire_time += more_time; }

            bool expired() const { 
               return get_current_time_in_ms() > _expire_time;
            }

            int remaining() const { return _expire_time - get_current_time_in_ms(); }

            bool reached() const { return expired(); }

         protected:
            int      _expire_time;
      };
   };
};

#endif // _RADIANT_PLATFORM_UTILS_H
