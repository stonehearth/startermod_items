local Point3 = _radiant.csg.Point3
local Fabricator = require 'components.fabricator.fabricator'

local TeardownStructureAdjacent = class()
TeardownStructureAdjacent.name = 'teardown structure adjacent'
TeardownStructureAdjacent.does = 'stonehearth:teardown_structure_adjacent'
TeardownStructureAdjacent.args = {
   fabricator = Fabricator,
   block = Point3,
}
TeardownStructureAdjacent.version = 2
TeardownStructureAdjacent.priority = 1

function TeardownStructureAdjacent:start(ai, entity, args)
   ai:set_status_text('tearing down structures...')
end

function TeardownStructureAdjacent:start_thinking(ai, entity, args)
   local carrying = ai.CURRENT.carrying
   if not carrying then
      ai:set_think_output()
      return
   end

   local material = args.fabricator:get_material()
   if not radiant.entities.is_material(carrying, material) then
      return
   end

   local item_component = carrying:get_component('item')
   if not item_component then
      return
   end
   if item_component:get_stacks() >= item_component:get_max_stacks() then
      return
   end
   ai:set_think_output()
end

function TeardownStructureAdjacent:run(ai, entity, args)
   local fabricator = args.fabricator
   local block = args.block
   
   radiant.entities.turn_to_face(entity, block)
   ai:execute('stonehearth:run_effect', { effect = 'work' })

   if fabricator:remove_block(block) then
      if not radiant.entities.increment_carrying(entity) then
         local oak_log = radiant.entities.create_entity('stonehearth:oak_log')
         local item_component = oak_log:add_component('item')
         item_component:set_stacks(1)
         radiant.entities.pickup_item(entity, oak_log)
      end
   end
end

return TeardownStructureAdjacent
