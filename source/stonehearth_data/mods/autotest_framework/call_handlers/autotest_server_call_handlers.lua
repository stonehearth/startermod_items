
local AutotestServerCallHandler = class()

AutotestServerCallHandler['fail'] = function (session, response)
   return error('remote autotest failure')
end

AutotestServerCallHandler['ui:connect_client_to_server'] = function (_, session, response)
   return autotest_framework.ui._connect_client_to_server(session, response)
end

return AutotestServerCallHandler