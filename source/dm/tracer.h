#ifndef _RADIANT_DM_TRACER_H
#define _RADIANT_DM_TRACER_H

#include "dm.h"

BEGIN_RADIANT_DM_NAMESPACE

class Tracer
{
public:
   virtual ~Tracer();

   enum TracerType {
      TRACER_SYNC,
      TRACER_BUFFERED,
      TRACER_STREAMER,
   };

   virtual TracerType GetType() const = 0;
   virtual void OnObjectChanged(ObjectId id) = 0;
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_TRACER_H

