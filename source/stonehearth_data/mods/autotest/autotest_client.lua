local _promise

local autotest = {
   ui = require 'lib.client.autotest_ui_client'
}

function autotest.fail(format, ...)
   _radiant.call_server('autotest:fail', string.format(format, ...))
end

return autotest
