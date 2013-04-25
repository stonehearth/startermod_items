#include "pch.h"
#include "actor_selector.h"
#include "client/renderer/renderer.h"

using namespace ::radiant;
using namespace ::radiant::client;

ActorSelector::ActorSelector(Selector::SelectionFn fn) :   
   _inputHandlerId(0)
{
   RegisterSelectionFn(fn);
   Activate();
}

ActorSelector::~ActorSelector()
{
   Deactivate();
}

void ActorSelector::Prepare()
{
}

void ActorSelector::RegisterSelectionFn(Selector::SelectionFn fn)
{
   _selectionFn = fn;
}

void ActorSelector::Activate()
{
   ASSERT(!_inputHandlerId);
   _selection.Clear();
   _inputHandlerId = Renderer::GetInstance().SetMouseInputCallback(std::bind(&ActorSelector::onInputEvent,
                                                                   this,
                                                                   std::placeholders::_1,
                                                                   std::placeholders::_2,
                                                                   std::placeholders::_3));
}

void ActorSelector::Deactivate()
{
   if (_inputHandlerId) {
      Renderer::GetInstance().RemoveInputEventHandler(_inputHandlerId);
      _inputHandlerId = 0;
   }
}

void ActorSelector::onInputEvent(const MouseInputEvent &evt, bool &handled, bool &uninstall)
{
   om::Selection s;

   if (evt.down[0]) {
      Renderer::GetInstance().QuerySceneRay(evt.x, evt.y, s);

      if (s.HasEntities()) {
         _selectionFn(s);
         handled = true;
      }
   }
}
