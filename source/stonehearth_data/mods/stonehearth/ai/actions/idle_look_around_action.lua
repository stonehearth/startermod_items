local IdleLookAround = class()

IdleLookAround.name = 'look around'
IdleLookAround.does = 'stonehearth:idle:bored'
IdleLookAround.args = { 
   hold_position = {    -- is the unit allowed to move around in the action?
      type = 'boolean',
      default = false,
   }
}
IdleLookAround.version = 2
IdleLookAround.priority = 1
IdleLookAround.weight = 2

function IdleLookAround:start_thinking(ai, entity)
   if not ai.CURRENT.carrying then
      ai:set_think_output()
   end
end

function IdleLookAround:run(ai, entity)
   ai:execute('stonehearth:run_effect', { effect = 'idle_look_around' })
end

return IdleLookAround
