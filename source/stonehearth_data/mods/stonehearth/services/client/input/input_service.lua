local log = radiant.log.create_logger('input_service')

local InputService = class()

function InputService:initialize()
  --self._sv = self.__saved_variables:get_data()

  self._mouse_handler_func_stack = {}
  self._kb_handler_func_stack = {}

  self._input_capture = _radiant.client.capture_input()
  self._input_capture:on_input(function(e)
    local data = e.mouse
    local stack = self._mouse_handler_func_stack

    if e.type == _radiant.client.Input.KEYBOARD then
      data = e.keyboard
      stack = self._kb_handler_func_stack
    end

    for i = #stack, 1, -1 do
      if stack[i](data) then
        return true
      end
    end
    return false
  end)
end

function InputService:_push_handler(stack, handler_func)
  stack[#stack + 1] = handler_func
end

function InputService:_remove_handler(stack, handler_func)
  local idx = 0

  for i = 1, #stack do
    if stack[i] == handler_func then
      idx = i
      break
    end
  end

  if idx ~= 0 then
    for i = idx, #stack - 1 do
      stack[idx] = stack[idx + 1]
    end
    stack[#stack] = nil
  end
end

function InputService:push_handlers(mouse_handler_func, kb_handler_func)
  local result = {
    mouse_handler = self:push_mouse_handler(mouse_handler_func),
    keyboard_handler = self:push_keyboard_handler(kb_handler_func)
  }
  return result
end

function InputService:remove_handlers(handler_bundle)
  self:remove_mouse_handler(handler_bundle.mouse_handler)
  self:remove_keyboard_handler(handler_bundle.keyboard_handler)
end

function InputService:push_mouse_handler(handler_func)
  self:_push_handler(self._mouse_handler_func_stack, handler_func)
  return handler_func
end

function InputService:push_keyboard_handler(handler_func)
  self:_push_handler(self._kb_handler_func_stack, handler_func)
  return handler_func
end

function InputService:remove_mouse_handler(handler_func)
  self:_remove_handler(self._mouse_handler_func_stack, handler_func)
end

function InputService:remove_keyboard_handler(handler_func)
  self:_remove_handler(self._kb_handler_func_stack, handler_func)
end

return InputService