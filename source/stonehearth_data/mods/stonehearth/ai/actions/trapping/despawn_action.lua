local Despawn = class()

Despawn.name = 'despawn'
Despawn.does = 'stonehearth:idle:bored'
Despawn.args = {
   hold_position = {    -- is the unit allowed to move around in the action?
      type = 'boolean',
      default = false,
   }   
}
Despawn.version = 2
Despawn.priority = 100

-- despawn when all work is done and entity has nothing to do
function Despawn:start_thinking(ai, entity, args)
   ai:set_think_output()
end

function Despawn:run(ai, entity, args)
   ai:execute('stonehearth:destroy_entity')
end

return Despawn
