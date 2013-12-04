#ifndef _RADIANT_DM_STREAM_H
#define _RADIANT_DM_STREAM_H

#include "dm.h"

BEGIN_RADIANT_DM_NAMESPACE

class Stream
{
public:
   virtual ~Stream() { }

   virtual void Flush(Protocol::* msg) = 0;
};

END_RADIANT_DM_NAMESPACE

#endif // _RADIANT_DM_STREAMER_H

