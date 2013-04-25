require 'copas'
local dkjson = require 'dkjson'
local ai_mgr = require 'radiant.ai.ai_mgr'
local log = require 'radiant.core.log'

local Endpoint = class()
function Endpoint:__init(entity_id, ws)
   self._entity_id = entity_id
   self._ws = ws
   self._info = {
      entity_id = entity_id
   }
   self._finished = not ai_mgr:add_debug_hook(entity_id, self)
end

function Endpoint:update()
   local cmd = 'fetch'
   local no_skt = { close = function () end }
   repeat
      if self._changed then 
         self._changed = false
         if self._co then
            self._info.stack = debug.traceback(self._co)
         else 
            self._info.stack = "no stack for this entity"
         end
         local msg = dkjson.encode(self._info)
         self._ws:send(msg)
         cmd = self._ws:receive()
      else
         copas.yield(no_skt)
      end
   until self._finished or cmd ~= 'fetch'   
end

function Endpoint:set_ai_priorities(info)
   self._info.activities = info
   self._changed = true
end

function Endpoint:notify_current_thread(co)
   self._co = co
   self._changed = true
end

local DebugWebSocket = class()
function DebugWebSocket:__init()
   self._endpoints = {}
   self._endpoint_id = 1
   self:_create_server()
   --self:_create_copas_thread()
end

function DebugWebSocket:_create_server()
   self._server = require 'websocket'.server.copas.listen {
      protocols = {
         ['game-ai'] = function(ws)
            copas.setErrorHandler(function (...) self:_on_error(...) end)
            
            local msg = ws:receive()
            local entity_id = tonumber(msg)
            local endpoint = Endpoint(entity_id, ws)
            endpoint:update()
            log:info('entity %d destroyed.  killing web socket', entity_id)
            ws:close()
         end
      },
      port = 13337
   }
end

function DebugWebSocket:_create_copas_thread()
   copas.addthread(function()
         copas.setErrorHandler(function (...) self:_on_error(...) end)
         local no_skt = { close = function () end }
         while true do
            self:_push_events()
            copas.yield(no_skt)
         end
      end)
end

function DebugWebSocket:_on_error(msg, co, skt)
   log:warning(msg)
   assert(false)
end

function DebugWebSocket:_push_events()
   for endpoint_id, endpoint in pairs(self._endpoints) do
      if endpoint:flush() then
         self._endpoints[endpoint_id] = nil
      end
   end
end

return DebugWebSocket()