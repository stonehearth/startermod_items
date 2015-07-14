local IdleMakeNoise = class()

IdleMakeNoise.name = 'make noise'
IdleMakeNoise.does = 'stonehearth:idle:bored'
IdleMakeNoise.args = { 
   hold_position = {    -- is the unit allowed to move around in the action?
      type = 'boolean',
      default = false,
   }
}
IdleMakeNoise.version = 2
IdleMakeNoise.priority = 1
IdleMakeNoise.weight = 1

function IdleMakeNoise:start_thinking(ai, entity)
   if not ai.CURRENT.carrying then
      ai:set_think_output()
   end
end

function IdleMakeNoise:run(ai, entity)
   ai:execute('stonehearth:run_effect', { effect = 'idle_make_noise' })
end

return IdleMakeNoise
