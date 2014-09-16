-- @title stonehearth:drop_carrying_adjacent
-- @book reference
-- @section activities

local Entity = _radiant.om.Entity

--[[ @markdown
You can trigger an event from a compound action like this: 

      return ai:create_compound_action(Foo)
         :execute(...)
         :execute('stonehearth:trigger_event', {
            source = event_key,
            event_name = 'stonehearth:event_name',
            event_args = {
               arg1 = ai.ENTITY,  --can use the AI args here
               arg2 = 'bar',
            },
         })
         :execute(...)
]]

local TriggerEvent = class()

TriggerEvent.name = 'trigger an event'
TriggerEvent.does = 'stonehearth:trigger_event'
TriggerEvent.args = {
   source = stonehearth.ai.ANY,  -- the source of the trigger
   event_name = 'string',        -- the event to trigger
   event_args = 'table',         -- the arguments to pass in the event
}
TriggerEvent.version = 2
TriggerEvent.priority = 1

-- Trigger an event given the parameters passed in through args (source, name, args)
function TriggerEvent:run(ai, entity, args)
   radiant.events.trigger_async(args.source, args.event_name, args.event_args)
end

return TriggerEvent
