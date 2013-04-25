local CommandHandler = class()

local dkjson = require 'dkjson'
local log = require 'radiant.core.log'
local check = require 'radiant.core.check'

function CommandHandler:__init()
   self._commands = {}
end

function CommandHandler:register_cmd(name, fn)
   check:is_string(name)
   check:is_function(fn)
   check:verify(not self._commands[name])
   
   self._commands[name] = fn
end

function CommandHandler:call(name, ...)
   check:is_string(name)
   check:verify(self._commands[name])
   
   log:info('executing command %s.', name)
   local result = self._commands[name](...)
   log:info('--> %s %s', type(result), tostring(result))
   check:is_table(result)
   return dkjson.encode(result), result
end

return CommandHandler()