local Point3 = _radiant.csg.Point3
local Fabricator = require 'components.fabricator.fabricator'

local FabricateStructureAdjacent = class()
FabricateStructureAdjacent.name = 'fabricate structure adjacent'
FabricateStructureAdjacent.does = 'stonehearth:fabricate_structure_adjacent'
FabricateStructureAdjacent.args = {
   fabricator = Fabricator,
   block = Point3,
}
FabricateStructureAdjacent.version = 2
FabricateStructureAdjacent.priority = 1

function FabricateStructureAdjacent:start_thinking(ai, entity, args)
   local material = args.fabricator:get_material()
   if not radiant.entities.is_material(ai.CURRENT.carrying, material) then
      return
   end
   ai:set_think_output()
end

function FabricateStructureAdjacent:run(ai, entity, args)
   self._fabricator = args.fabricator
   self._current_block = args.block
    
   local carrying = radiant.entities.get_carrying(entity)
   local standing = radiant.entities.get_world_grid_location(entity)
   repeat
      radiant.entities.turn_to_face(entity, self._current_block)
      ai:execute('stonehearth:run_effect', { effect = 'work' })
      if not self._fabricator:add_block(carrying, self._current_block) then
         ai:abort('failed to add block to fabricator')
      end

      carrying = radiant.entities.consume_carrying(entity)
      self._current_block = self._fabricator:find_another_block(carrying, standing)
   until not self._current_block
end

function FabricateStructureAdjacent:start(ai, entity)
   ai:set_status_text('building...')
end

function FabricateStructureAdjacent:stop(ai, entity)
   if self._current_block then
      self._fabricator:release_block(self._current_block)
      self._current_block = nil
   end
end


return FabricateStructureAdjacent
