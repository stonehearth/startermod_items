local Entity = _radiant.om.Entity
local CallFunction = class()

CallFunction.name = 'call a function'
CallFunction.does = 'stonehearth:call_function'
CallFunction.args = {
   fn = 'function',     -- the function to call
   args = {             -- arguments to the function
      type = 'table',
      default = {}
   }
}
CallFunction.version = 2
CallFunction.priority = 1

function CallFunction:run(ai, entity, args)
   args.fn(unpack(args.args))
end

return CallFunction
