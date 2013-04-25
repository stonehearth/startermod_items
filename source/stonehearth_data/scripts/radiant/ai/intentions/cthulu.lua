local Cthulu = class()
local om = require 'radiant.core.om'
local ai_mgr = require 'radiant.ai.ai_mgr'
ai_mgr:register_action('radiant.actions.cthulu', Cthulu)
ai_mgr:register_intention('radiant.intentions.cthulu', Cthulu)

function Cthulu:recommend_activity(entity)
   self._done = false
   if not self._done then
      return 999, { 'radiant.actions.cthulu'}
   end
end

function Cthulu:run(ai, entity)
   local height = om:get_component(entity, 'mob'):get_world_location().y
   ai:execute('radiant.actions.goto_location', RadiantPoint3(273, height, 211))
   om:turn_to_face(entity, RadiantIPoint3(223, 1, 225))
   ai:execute('radiant.actions.perform', 'roar')
   self._done = true
end
