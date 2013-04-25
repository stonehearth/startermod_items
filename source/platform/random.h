#ifndef _RADIANT_PLATFORM_RANDOM_H
#define _RADIANT_PLATFORM_RANDOM_H

#define _USE_MATH_DEFINES
#include <math.h>
#include "namespace.h"

BEGIN_RADIANT_PLATFORM_NAMESPACE

class Random{
   public:
      int Roll(int size) {
         return (rand() % size) + 1;      
      }

      int Range(int low, int high) {
         return low + Roll(high - low + 1) - 1;
      }

      float RandomAngleRadians() {
         return (float)(2 * M_PI * rand() / RAND_MAX);
      }
};

END_RADIANT_PLATFORM_NAMESPACE

#endif // _RADIANT_PLATFORM_RANDOM_H
