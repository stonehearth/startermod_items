local log = radiant.log.create_logger('input_service')

local InputCapture = require 'services.client.input.input_capture'
local InputService = class()

function InputService:initialize()
  -- stack of all the input handlers.
  self._stack = {}

  self._input_capture = _radiant.client.capture_input()  
  self._input_capture:on_input(function(e)
    return self:_dispatch_input(e)
  end)
end

-- return a new InputCapture object and put it at the back of the stack.
function InputService:capture_input()
  local capture = InputCapture()
  table.insert(self._stack, capture)
  log:debug('adding input capture to stack (len:%d)', #self._stack)
  return capture
end

-- private method to dispatch an input.  note that this is called
-- externally by the autotest framework (see mouse_capture.lua).
-- so if you're going to change it or move it, make sure you don't
-- break the autotests!
function InputService:_dispatch_input(e)
  -- walk down the stack looking for a capture object which handles
  -- the event
  for i = #self._stack, 1, -1 do 
    local capture = self._stack[i]
    -- if the capture object has been destroyed, remove it from the
    -- stack.  table.remove will compress the array so there are no
    -- holes.  we've already processed the ones above i, so no one
    -- will get skipped
    if capture:_is_destroyed() then
      log:debug('removing input capture at index %d', i)
      table.remove(self._stack, i)
    else
      -- if this capture object handles the event, stop processing the
      -- event propagation
      if capture:_dispatch(e) then
        log:detail('input capture %d handled input.', i)
        return true
      end
    end
  end
  return false
end

return InputService
