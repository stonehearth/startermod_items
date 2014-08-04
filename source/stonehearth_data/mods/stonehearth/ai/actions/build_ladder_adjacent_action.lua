local Entity = _radiant.om.Entity
local LadderBuilder = require 'services.server.build.ladder_builder'

local BuildLadderAdjacent = class()
BuildLadderAdjacent.name = 'build ladder adjacent'
BuildLadderAdjacent.does = 'stonehearth:build_ladder_adjacent'
BuildLadderAdjacent.args = {
   ladder = Entity,           -- the ladder to build
   builder = LadderBuilder,   -- the ladder builder class
}
BuildLadderAdjacent.version = 2
BuildLadderAdjacent.priority = 1

function BuildLadderAdjacent:start_thinking(ai, entity, args)
   local material = args.builder:get_material()
   if not radiant.entities.is_material(ai.CURRENT.carrying, material) then
      return
   end
   ai:set_think_output()
end

function BuildLadderAdjacent:run(ai, entity, args)
   local ladder = args.ladder
   local builder = args.builder
   local carrying = radiant.entities.get_carrying(entity)

   while carrying and not builder:is_ladder_finished() do
      radiant.entities.turn_to_face(entity, ladder)
      ai:execute('stonehearth:run_effect', { effect = 'work' })
      builder:grow_ladder()
      carrying = radiant.entities.consume_carrying(entity)
   end
end

function BuildLadderAdjacent:start(ai, entity)
   ai:set_status_text('building ladder...')
end

return BuildLadderAdjacent
