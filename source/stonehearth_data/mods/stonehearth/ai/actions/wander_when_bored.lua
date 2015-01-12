local WanderWhenBored = class()

WanderWhenBored.name = 'wander when bored'
WanderWhenBored.does = 'stonehearth:idle:bored'
WanderWhenBored.args = {
   hold_position = {    -- is the unit allowed to move around in the action?
      type = 'boolean',
      default = false,
   }   
}
WanderWhenBored.fixed_cost = 0 -- only for compound actions.  ignore the cost of actually wandering!
WanderWhenBored.version = 2
WanderWhenBored.priority = 1
WanderWhenBored.weight = 2

function WanderWhenBored:start_thinking(ai, entity, args)
   if not args.hold_position then
      ai:set_think_output()
   end
end

local ai = stonehearth.ai
return ai:create_compound_action(WanderWhenBored)
         :execute('stonehearth:wander_within_leash', { radius = 5, radius_min = 2 })
