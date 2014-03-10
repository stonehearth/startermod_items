local ResponseQueue = class()

function ResponseQueue:__init(response)
   self._response = response
   self._queue = {}
end

function ResponseQueue:set_response(response)
   self._response = response
   self:_flush()
end

function ResponseQueue:send(...)
   table.insert(self._queue, {...})
   self:_flush()
end

function ResponseQueue:_flush()
   if self._response and #self._queue > 0 then
      for _, msg in ipairs(self._queue) do
         self._response:notify(msg)
      end
      self._queue = {}
   end
end
 
return ResponseQueue

