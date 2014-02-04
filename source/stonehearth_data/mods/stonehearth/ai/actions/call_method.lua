local CallMethod = class()

CallMethod.name = 'call a method on an object'
CallMethod.does = 'stonehearth:call_method'
CallMethod.args = {
   obj = stonehearth.ai.ANY,  -- the object to call a method on
   method = 'string',         -- the method to call
   args = 'table',            -- arguments to the function
}
CallMethod.version = 2
CallMethod.priority = 1

function CallMethod:run(ai, entity, args)
   args.obj[args.method](args.obj, unpack(args.args))
end

return CallMethod
