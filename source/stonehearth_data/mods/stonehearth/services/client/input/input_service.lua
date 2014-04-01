local log = radiant.log.create_logger('input_service')

local InputService = class()

function InputService:initialize()
  --self._sv = self.__saved_variables:get_data()

  self._handler_func_stack = {}

  self._input_capture = _radiant.client.capture_input()
  self._input_capture:on_input(function(e)
    if e.type == _radiant.client.Input.MOUSE then
      for i = #self._handler_func_stack, 1, -1 do

        if self._handler_func_stack[i](e.mouse) then
          return true
        end
      end
    end
    return false
  end)
end

function InputService:push_handler(handler_func)
  local f = handler_func
  self._handler_func_stack[#self._handler_func_stack + 1] = f
end

function InputService:remove_handler(handler_func)
  local idx = 0

  for i = 1, #self._handler_func_stack do
    if self._handler_func_stack[i] == handler_func then
      idx = i
      break
    end
  end

  if idx ~= 0 then
    for i = idx, #self._handler_func_stack - 1 do
      self._handler_func_stack[idx] = self._handler_func_stack[idx + 1]
    end

    self._handler_func_stack[#self._handler_func_stack] = nil
  end
end

return InputService