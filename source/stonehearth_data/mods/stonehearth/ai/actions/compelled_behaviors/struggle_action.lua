local StruggleAction = class()

StruggleAction.name = 'struggle'
StruggleAction.does = 'stonehearth:compelled_behavior'
StruggleAction.args = {}
StruggleAction.version = 2
StruggleAction.priority = stonehearth.constants.priorities.compelled_behavior.STRUGGLE

function StruggleAction:run(ai, entity)
   -- todo, insert a saving throw
   while true do
      ai:execute('stonehearth:run_effect', { effect = 'idle_look_around'})
   end
end

return StruggleAction
