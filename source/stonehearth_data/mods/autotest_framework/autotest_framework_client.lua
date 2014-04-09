
local autotest_framework = {
   ui = require 'lib.client.autotest_ui_client',
   log = radiant.log.create_logger('autotest'),
}

function autotest_framework.fail(format, ...)
   _radiant.call_server('autotest_framework:fail', string.format(format, ...))
end

return autotest_framework
