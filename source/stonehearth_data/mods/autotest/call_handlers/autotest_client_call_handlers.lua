
local AutotestClientCallHandler = class()

AutotestClientCallHandler['ui:connect_browser_to_client'] = function (_, session, response)
   return autotest.ui._connect_browser_to_client(session, response)
end

return AutotestClientCallHandler