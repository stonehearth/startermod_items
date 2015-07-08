local IdleTop = class()

IdleTop.name = 'idle top'
IdleTop.does = 'stonehearth:top'
IdleTop.args = {}
IdleTop.version = 2
IdleTop.priority = stonehearth.constants.priorities.top.IDLE

function IdleTop:run(ai, entity)
   local log = ai:get_log()
   local ai_component = entity:get_component('stonehearth:ai')
   local stale

   repeat
      ai:execute('stonehearth:idle')
      stale = ai_component:entity_state_is_stale()
   until not stale
   log:detail('leaving stonehearth:idle (entity_state_stale:%s)', stale)
end

return IdleTop
