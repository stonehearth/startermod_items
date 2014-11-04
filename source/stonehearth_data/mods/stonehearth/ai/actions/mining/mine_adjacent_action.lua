local Entity = _radiant.om.Entity
local Point3 = _radiant.csg.Point3
local MineAdjacent = class()

MineAdjacent.name = 'mine adjacent'
MineAdjacent.does = 'stonehearth:mining:mine_adjacent'
MineAdjacent.args = {
   mining_zone = Entity,
   point_of_interest = Point3,
}
MineAdjacent.version = 2
MineAdjacent.priority = 1

function MineAdjacent:start_thinking(ai, entity, args)
   if not ai.CURRENT.carrying and not radiant.entities.get_carrying(entity) then
      ai:set_think_output()
   end
end

function MineAdjacent:run(ai, entity, args)
   radiant.entities.turn_to_face(entity, args.point_of_interest)
   ai:execute('stonehearth:run_effect', { effect = 'mine' })

   ai:unprotect_entity(args.mining_zone)

   local mining_zone_component = args.mining_zone:add_component('stonehearth:mining_zone')
   local loot = mining_zone_component:mine_point(args.point_of_interest)
   local location = radiant.entities.get_world_grid_location(entity)
   local items = radiant.entities.spawn_items(loot, location, 1, 3)
end

return MineAdjacent
