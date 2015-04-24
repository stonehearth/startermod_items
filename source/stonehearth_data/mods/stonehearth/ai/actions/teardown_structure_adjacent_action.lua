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

local MATERIAL_TO_RESOURCE = {
   ['wood resource']  = 'stonehearth:resources:wood:oak_log',
   ['stone resource'] = 'stonehearth:resources:resources:stone:hunk_of_stone',
}

function TeardownStructureAdjacent:start(ai, entity, args)
   ai:set_status_text('tearing down structures...')
end

function TeardownStructureAdjacent:start_thinking(ai, entity, args)
   self._material = args.fabricator:get_material(args.block)
   if not self._material then
      ai:halt(string.format('point %s is not in fabricator in teardown adjacent action', tostring(args.block)))
      return
   end  
   
   local carrying = ai.CURRENT.carrying
   if not carrying then
      ai:set_think_output()
      return
   end
   if not radiant.entities.is_material(carrying, self._material) then
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
         local resource = radiant.entities.create_entity(MATERIAL_TO_RESOURCE[self._material])
         resource:add_component('item')
                     :set_stacks(1)
         radiant.entities.pickup_item(entity, resource)
      end
   end
end

return TeardownStructureAdjacent
