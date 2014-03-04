local commands = require 'lib.common.autotest_ui_commands'
local ResponseQueue = require 'lib.common.response_queue'

local _client = ResponseQueue()
local ui_server = {}

function ui_server.push_unitframe_command_button(entity, command)
   local jq_selector = '#unitFrame #commandButtons #'.. command
   _client:send(commands.SELECT_ENTITY, entity)
   _client:send(commands.WAIT_REALTIME, 1000)
   _client:send(commands.CLICK_DOM_ELEMENT, jq_selector)
end

function ui_server.sleep(ms)
   _client:send(commands.WAIT_REALTIME, ms)
end

function ui_server.click_dom_element(jq_selector)
   _client:send(commands.CLICK_DOM_ELEMENT, jq_selector)
end

function ui_server._fail_from_client(session, response, msg)
   autotest.fail('automation client requested abort: "%s"', msg)
end

function ui_server._connect_client_to_server(session, response)
   _client:set_response(response)
end

return ui_server
