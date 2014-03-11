
local AutotestServerCallHandler = class()

AutotestServerCallHandler['fail'] = function (session, response)
   return autotest.fail()
end

AutotestServerCallHandler['ui:connect_client_to_server'] = function (_, session, response)
   return autotest.ui._connect_client_to_server(session, response)
end

return AutotestServerCallHandler