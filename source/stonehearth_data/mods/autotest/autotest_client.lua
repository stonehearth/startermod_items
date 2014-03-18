local _promise

local autotest = {
   ui = require 'lib.client.autotest_ui_client'
}

function autotest.log(format, ...) 
   radiant.log.write('autotest', 0, format, ...)
end

function autotest.fail(format, ...)
   _radiant.call_server('autotest:fail', string.format(format, ...))
end

return autotest
