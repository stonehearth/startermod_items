local InputCapture = class()

function InputCapture:destroy()
  self._destroyed = true
end

-- set the mouse event handler
function InputCapture:on_mouse_event(cb)
  self._mouse_cb = cb
  return self
end

-- set the keyboard event handler
function InputCapture:on_keyboard_event(cb)
  self._keyboard_cb = cb
  return self
end

-- private method used by the InputService to check to see
-- if the input capture has been destroyed.
function InputCapture:_is_destroyed()
  return self._destroyed
end

-- dispatch the event to one of our registered handlers
function InputCapture:_dispatch(e)  
  if e.type == _radiant.client.Input.MOUSE then
    if self._mouse_cb then
      return self._mouse_cb(e.mouse)
    end
  elseif e.type == _radiant.client.Input.KEYBOARD then
    if self._keyboard_cb then
      return self._keyboard_cb(e.keyboard)
    end
  end
  return false
end

return InputCapture

