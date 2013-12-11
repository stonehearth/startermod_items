local commands = {}
local singleton = {}

local log = radiant.log.create_logger('commands')

function commands.__init()
   singleton._commands = {}
end

function commands.register(name, fn)
   radiant.check.is_string(name)
   radiant.check.is_function(fn)
   radiant.check.verify(not singleton._commands[name])
   
   singleton._commands[name] = fn
end

function commands.call(name, ...)
   radiant.check.is_string(name)
   radiant.check.verify(singleton._commands[name])
   
   log:debug('executing command %s.', name)

   local result = singleton._commands[name](...)
   log:debug('command returned --> %s %s', type(result), tostring(result))
   radiant.check.is_table(result)

   return radiant.json.encode(result), result
end

commands.__init()
return commands