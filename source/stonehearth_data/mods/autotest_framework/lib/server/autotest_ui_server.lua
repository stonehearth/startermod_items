local Point2 = _radiant.csg.Point2
local Point3 = _radiant.csg.Point3
local commands = require 'lib.common.autotest_ui_commands'
local ResponseQueue = require 'lib.common.response_queue'

local ui_sleep = 400
local _client = ResponseQueue()
local ui_server = {}

-- stall to simulate user lag and let the javascript have a time to chew
local function _send(...)
   _client:send(...)
   --_client:send(commands.WAIT_REALTIME, 1000)
end

function ui_server.sleep(ms)
   -- don't use _send here to avoid the simulated user paus
   _client:send(commands.WAIT_REALTIME, ms)
end

function ui_server.move_camera(position, look_at)
   _client:send(commands.MOVE_CAMERA, position, look_at)
end

function ui_server.click_terrain(x, z)
   local y = radiant.terrain.get_height(x, z)
   _send(commands.CLICK_WORLD_COORD, x, y, z)
   _client:send(commands.WAIT_REALTIME, ui_sleep)
end

function ui_server.set_next_designation_region(x1, z1, w, h)
   local x2, z2 = x1 + w, z1 + h
   local y1 = radiant.terrain.get_height(x1, z1)
   local p0 = Point3(x1, y1, z1)
   local p1 = Point3(x2, y1 + 1, z2)
   _send(commands.SET_SELECT_XZ_REGION, p0, p1)
   _client:send(commands.WAIT_REALTIME, ui_sleep)
end

function ui_server.push_unitframe_command_button(entity, command)
   local jq_selector = '#unitFrame #commandButtons #'.. command
   _send(commands.SELECT_ENTITY, entity)
   _client:send(commands.WAIT_REALTIME, ui_sleep)
   _send(commands.CLICK_DOM_ELEMENT, jq_selector)
   _client:send(commands.WAIT_REALTIME, ui_sleep)
end

function ui_server.click_dom_element(jq_selector)
   _send(commands.CLICK_DOM_ELEMENT, jq_selector)
   _client:send(commands.WAIT_REALTIME, ui_sleep)
end

function ui_server._connect_client_to_server(session, response)
   _client:set_response(response)
end

return ui_server
