local Entity = _radiant.om.Entity
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

function TriggerEvent:run(ai, entity, args)
   radiant.events.trigger_async(args.source, args.event_name, args.event_args)
end

return TriggerEvent
