#ifndef _RADIANT_CLIENT_MOUSE_EVENT_PROMISE_H
#define _RADIANT_CLIENT_MOUSE_EVENT_PROMISE_H

#include "radiant_file.h"
#include "namespace.h"

BEGIN_RADIANT_CLIENT_NAMESPACE

class MouseEventPromise
{
public:
   MouseEventPromise();
   ~MouseEventPromise();

   typedef std::function<bool(const MouseEvent &mouse)> MouseEventHandlerFn;

   void OnMouseEvent(const MouseEvent &mouse, bool &handled);
   void TraceMouseEvent(MouseEventHandlerFn cb);
   bool IsActive() const;
   void Uninstall();

private:
   bool                             active_;
   std::vector<MouseEventHandlerFn> cbs_;
};

typedef std::shared_ptr<MouseEventPromise> MouseEventPromisePtr;
typedef std::weak_ptr<MouseEventPromise> MouseEventPromiseRef;

END_RADIANT_CLIENT_NAMESPACE

#endif // _RADIANT_CLIENT_MOUSE_EVENT_PROMISE_H
