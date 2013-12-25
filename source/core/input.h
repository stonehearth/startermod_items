#ifndef _RADIANT_INPUT_H
#define _RADIANT_INPUT_H

#include <istream>

namespace radiant {
   struct MouseInput {
      int         wheel;
      int         x;
      int         y;
      int         dx;
      int         dy;
      int         drag_start_x;
      int         drag_start_y;
      bool        dragging;
      bool        dragend;
      bool        in_client_area;
      bool        up[16];
      bool        down[16];
      bool        buttons[16];
   };

   struct KeyboardInput {
      bool        shift;
      bool        ctrl;
      bool        down;
      int         key;
   };

   struct RawInput {
      UINT     msg;
      WPARAM   wp;
      LPARAM   lp;
   };

   struct Input {
      enum Bits {
         MOUSE = 1,
         KEYBOARD = 2,
         RAW_INPUT = 3,
      };
      int            type;
      MouseInput     mouse;
      KeyboardInput  keyboard;
      RawInput       raw_input;     
      bool           focused;
   };
};

#endif // _RADIANT_JSON_H
