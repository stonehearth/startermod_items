local Entity = _radiant.om.Entity
local Point3 = _radiant.csg.Point3

local DropItemNearWorkshop = class()
DropItemNearWorkshop.name = 'drop item near workshop'
DropItemNearWorkshop.does = 'stonehearth:clear_workshop'
DropItemNearWorkshop.args = {
   item = Entity
}
DropItemNearWorkshop.version = 2
DropItemNearWorkshop.priority = 1 --Lower priority than dropping the item off at a nearby stockpile

function DropItemNearWorkshop:start(ai, entity, args)
   ai:set_status_text('clearing workshop')
end

--Find an empty space nearby to drop the item
--TODO: if this takes a moment, then the carpenter may wander before actually finding the location
--but at least it fixes the bug where the carpenter will put something back down on his workbench and get stufk
function DropItemNearWorkshop:start_thinking(ai, entity, args)
   local origin = radiant.entities.get_world_grid_location(entity)
   ai.CURRENT.location = radiant.terrain.find_placement_point(origin, 2, 5)
   if ai.CURRENT.location then
      ai:set_think_output()
   end
end

local ai = stonehearth.ai
return ai:create_compound_action(DropItemNearWorkshop)
   :execute('stonehearth:pickup_item_on_table', { item = ai.ARGS.item })
   :execute('stonehearth:drop_carrying_at', {location = ai.CURRENT.location})
