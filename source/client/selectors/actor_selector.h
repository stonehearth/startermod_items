#ifndef _RADIANT_CLIENT_ACTOR_SELECTOR_H
#define _RADIANT_CLIENT_ACTOR_SELECTOR_H

#include <map>
#include "namespace.h"
#include "selector.h"
#include "om/selection.h"

BEGIN_RADIANT_CLIENT_NAMESPACE

class OgreRenderer;

class ActorSelector : public Selector {
   public:
      ActorSelector(Selector::SelectionFn fn);
      virtual ~ActorSelector();

   public:
      void Deactivate() override;
      void Prepare() override;
      void RegisterSelectionFn(Selector::SelectionFn fn) override;

   public:
      void Activate();

   protected:
      void onInputEvent(const MouseEvent &evt, bool &handled, bool &uninstall);

   protected:
      Selector::SelectionFn   _selectionFn;
      int                     _inputHandlerId;
      om::Selection           _selection;
};

END_RADIANT_CLIENT_NAMESPACE

#endif //  _RADIANT_CLIENT_ACTOR_SELECTOR_H
