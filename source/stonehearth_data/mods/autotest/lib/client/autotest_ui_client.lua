local Point3 = _radiant.csg.Point3
local Point3f = _radiant.csg.Point3f

local commands = require 'lib.common.autotest_ui_commands'
local ResponseQueue = require 'lib.common.response_queue'
local RequestDispatcher = require 'lib.common.request_dispatcher'

local _mouse_capture = require 'lib.common.mouse_capture'
local _select_xz_region = _radiant.client._select_xz_region
local _browser = ResponseQueue()
local _server = RequestDispatcher()
local _browser_cmd_queue = {}

_server.on[commands.WAIT_REALTIME] = function(ms)
   _server:pause_for_realtime(ms)
end

_server.on[commands.SELECT_ENTITY] = function(entity)
   if not entity or not entity:is_valid() then
      _fail('could not select invalid entity')
   end
   autotest.log('selecting entity: %s', tostring(entity))
   _radiant.client.select_entity(entity)
end

_server.on[commands.CLICK_DOM_ELEMENT] = function(jq_selector)
   autotest.log('clicking on DOM element "%s"', jq_selector)
   _browser:send(commands.CLICK_DOM_ELEMENT, jq_selector)
end

_server.on[commands.CLICK_WORLD_COORD] = function(x, y, z)
   autotest.log('clicking on world coodrinate (%d, %d, %d)', x, y, z)
   _mouse_capture.click(x, y, z)
end

_server.on[commands.SET_SELECT_XZ_REGION] = function(p0, p1)
   local p0 = Point3(p0.x, p0.y, p0.z)
   local p1 = Point3(p1.x, p1.y, p1.z)
   autotest.log('setting xz region to %s - %s', p0, p1)
   _mouse_capture.set_select_xz_region(p0, p1)
end

_server.on[commands.MOVE_CAMERA] = function(position, look_at)
   stonehearth.camera:set_position(Point3f(position.x, position.y, position.z), true)
   stonehearth.camera:look_at(Point3f(look_at.x, look_at.y, look_at.z))
end

local ui_client = {}
function ui_client._connect_browser_to_client(session, response)
   _browser:set_response(response)
end

function ui_client.connect()
   _mouse_capture.hook()
   
   -- pick a better start point...
   local CAMERA_POSITION = Point3f(10.44, 16.90, 39.23)
   local CAMERA_LOOK_AT = Point3f(3.5, 1, 12.5)
   stonehearth.camera:set_position(CAMERA_POSITION, true)
   stonehearth.camera:look_at(CAMERA_LOOK_AT)

   _server:connect('autotest:ui:connect_client_to_server')
end

return ui_client
