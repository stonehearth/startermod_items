#ifndef _RADIANT_DM_TRACER_H
#define _RADIANT_DM_TRACER_H

#include "dm.h"

BEGIN_RADIANT_DM_NAMESPACE

class Tracer
{
public:
   Tracer(std::string const& name);
   virtual ~Tracer();

   enum TracerType {
      SYNC,
      BUFFERED,
      STREAMER,
   };

private:
   friend Store;
   virtual TracerType GetType() const = 0;

protected:
   std::string const& GetName () const { return name_; }

protected:
   std::string                   name_;
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_TRACER_H

