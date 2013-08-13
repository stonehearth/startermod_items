#include "pch.h"
#include "mouse_event_promise.h"

using namespace ::radiant;
using namespace ::radiant::client;

MouseEventPromise::MouseEventPromise() :
   active_(true)
{
}

MouseEventPromise::~MouseEventPromise()
{
}

void MouseEventPromise::OnMouseEvent(const MouseEvent &mouse, bool &handled)
{
   for (auto const &cb : cbs_) {
      if (handled) {
         break;
      }
      handled = cb(mouse);
   }
}

void MouseEventPromise::TraceMouseEvent(MouseEventHandlerFn cb)
{
   cbs_.emplace_back(cb);
}

void MouseEventPromise::Uninstall()
{
   active_ = false;
}

bool MouseEventPromise::IsActive() const
{
   return active_;
}
