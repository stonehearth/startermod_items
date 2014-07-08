local log = radiant.log.create_logger('combat')

local DefendNop = class()

DefendNop.name = 'defend nop'
DefendNop.does = 'stonehearth:combat:defend'
DefendNop.args = {
   assault_events = 'table'
}
DefendNop.version = 2
DefendNop.priority = 1   -- DefendNop should have the lowest priority of all defenses

-- DefendNop is used to resolve to an assault event when no other defense is available.
-- This allows the task to be cleared and for the WaitForAssault action to listen for the next assault event.
function DefendNop:start_thinking(ai, entity, args)
   ai:set_think_output()
end

function DefendNop:run(ai, entity, args)
   -- do nothing, defined only for the debugger breakpoint
   local breakpoint = 1;
end

return DefendNop
