#ifndef _RADIANT_DM_TRACER_H
#define _RADIANT_DM_TRACER_H

#include "dm.h"

BEGIN_RADIANT_DM_NAMESPACE

class Tracer
{
public:
   Tracer(int category) :
      category_(category)
   {
   }

   virtual ~Tracer();

   enum TracerType {
      SYNC,
      BUFFERED,
      STREAMER,
   };

   int GetCategory() const
   {
      return category_;
   }

   virtual TracerType GetType() const = 0;
   virtual void OnObjectChanged(ObjectId id) = 0;

private:
   int   category_;
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_TRACER_H

