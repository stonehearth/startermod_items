#ifndef _RADIANT_CLIENT_INPUT_EVENT_H
#define _RADIANT_CLIENT_INPUT_EVENT_H

#include "namespace.h"
#include "Horde3DRadiant.h"
#include "glfw.h"

#define MAX_MOUSE_BUTTONS     16

BEGIN_RADIANT_CLIENT_NAMESPACE

typedef unsigned int InputCallbackId;

class MouseInputEvent {
   public:
      int         wheel;
      int         x;
      int         y;
      int         dx;
      int         dy;
      int         drag_start_x;
      int         drag_start_y;
      bool        dragging;
      bool        dragend;
      bool        up[GLFW_MOUSE_BUTTON_LAST + 1];
      bool        down[GLFW_MOUSE_BUTTON_LAST + 1];
      bool        buttons[GLFW_MOUSE_BUTTON_LAST + 1];
};

class KeyboardEvent {
   public:
      bool        down;
      int         key;
};

class RawInputEvent {
   public:
      UINT     msg;
      WPARAM   wp;
      LPARAM   lp;
};

typedef std::function<void (const RawInputEvent&, bool& handled, bool& uninstall)> RawInputEventCb;
typedef std::function<void (const MouseInputEvent&, bool& handled, bool& uninstall)> MouseInputEventCb;
typedef std::function<void (const KeyboardEvent&, bool& handled, bool& uninstall)> KeyboardInputEventCb;

END_RADIANT_CLIENT_NAMESPACE

#endif //  _RADIANT_CLIENT_MouseEvent_H
