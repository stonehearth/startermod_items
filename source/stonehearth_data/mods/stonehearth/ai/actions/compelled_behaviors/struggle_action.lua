local rng = _radiant.csg.get_default_rng()

local StruggleAction = class()

StruggleAction.name = 'struggle'
StruggleAction.does = 'stonehearth:compelled_behavior'
StruggleAction.args = {}
StruggleAction.version = 2
StruggleAction.priority = stonehearth.constants.priorities.compelled_behavior.STRUGGLE

function StruggleAction:run(ai, entity)
   while true do
      local facing = rng:get_int(0, 3) * 90
      radiant.entities.turn_to(entity, facing)
      ai:execute('stonehearth:run_effect', { effect = 'idle_look_around'})
   end
end

return StruggleAction
