local Entity = _radiant.om.Entity
local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local Region3 = _radiant.csg.Region3
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
      -- ai.CURRENT.location is the official location used by the mining code to determine reachability
      -- The movement code may stop before reaching this location because the entity's reach is sufficient
      -- to hit the block earlier and because the mining animation looks better when the worker's face isn't
      -- buried in the wall.
      self._adjacent_location = ai.CURRENT.location
      ai:set_think_output()
   end
end

function MineAdjacent:run(ai, entity, args)
   local mining_zone = args.mining_zone
   local mining_zone_component = args.mining_zone:add_component('stonehearth:mining_zone')
   local destination_component = args.mining_zone:add_component('destination')
   local zone_location = radiant.entities.get_world_grid_location(mining_zone)
   -- the actual location of the worker, which typically stops short of moving completely to the adjacent
   local worker_location = radiant.entities.get_world_grid_location(entity)

   ai:unprotect_argument(args.mining_zone)
   self._current_block = args.point_of_interest

   while self._current_block do
      self._reserved_region = stonehearth.mining:get_reserved_region_for_poi(self._current_block, self._adjacent_location, mining_zone)
      self._reserved_region:translate(-zone_location)
      destination_component:get_reserved():modify(function(cursor)
            cursor:add_region(self._reserved_region)
         end)

      radiant.entities.turn_to_face(entity, self._current_block)
      ai:execute('stonehearth:run_effect', { effect = 'mine' })
      local loot = mining_zone_component:mine_point(self._current_block)
      local items = radiant.entities.spawn_items(loot, worker_location, 1, 3, { owner = entity })

      -- for the autotest
      radiant.events.trigger(entity, 'stonehearth:mined_location', { location = self._current_block })

      if mining_zone:is_valid() then
         -- The reserved region may include support blocks. We must release it before looking
         -- for the next block to mine so that they will be included as candidates.
         destination_component:get_reserved():modify(function(cursor)
               cursor:subtract_region(self._reserved_region)
               self._reserved_region = nil
            end)
         self._current_block = stonehearth.mining:resolve_point_of_interest(self._adjacent_location, mining_zone)
         -- prevent the worker from mining the block he is currently standing on
         if self._current_block and self._current_block == worker_location - Point3.unit_y then
            self._current_block = nil
         end
      else
         self._current_block = nil
      end
   end
end

function MineAdjacent:stop(ai, entity, args)
   if self._reserved_region and args.mining_zone:is_valid() then
      local destination_component = args.mining_zone:add_component('destination')

      destination_component:get_reserved():modify(function(cursor)
            cursor:subtract_region(self._reserved_region)
         end)
   end

   self._reserved_region = nil
   self._current_block = nil
   self._adjacent_location = nil
end

return MineAdjacent
