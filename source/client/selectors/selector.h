#ifndef _RADIANT_CLIENT_SELECTOR_H
#define _RADIANT_CLIENT_SELECTOR_H

#include "namespace.h"
#include "om/om.h"

BEGIN_RADIANT_CLIENT_NAMESPACE

class Selector {
   public:
      typedef std::function < void ( const om::Selection & ) > SelectionFn;

      virtual void Prepare() = 0;
      virtual void Deactivate() = 0;
      virtual void RegisterSelectionFn(SelectionFn fn) = 0;
};

END_RADIANT_CLIENT_NAMESPACE

#endif //  _RADIANT_CLIENT_SELECTOR_H
