local commands = require 'lib.common.autotest_ui_commands'
local ResponseQueue = require 'lib.common.response_queue'
local RequestDispatcher = require 'lib.common.request_dispatcher'

local _mouse_capture = require 'lib.common.mouse_capture'
local _select_xz_region = _radiant.client._select_xz_region
local _browser = ResponseQueue()
local _server = RequestDispatcher()

_server.on[commands.WAIT_REALTIME] = function(ms)
   _server:pause_for_realtime(ms)
end

_server.on[commands.SELECT_ENTITY] = function(entity)
   if not entity or not entity:is_valid() then
      _fail('could not select invalid entity')
   end
   _radiant.client.select_entity(entity)
end

_server.on[commands.CLICK_DOM_ELEMENT] = function(jq_selector)
   _browser:send(commands.CLICK_DOM_ELEMENT, jq_selector)
end

_server.on[commands.CLICK_TERRAIN] = function(x, y, z)
   _mouse_capture.click(x, y, z)
end

local ui_client = {}
function ui_client._connect_browser_to_client(session, response)
   _browser:set_response(response)
end

function ui_client.connect()
   local promise = _radiant.call('autotest:ui:connect_client_to_server')
   _server:set_promise(promise)
end

_mouse_capture.hook()

return ui_client
