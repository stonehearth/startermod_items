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

function MineAdjacent:start(ai, entity, args)
   self._destination_component = args.mining_zone:add_component('destination')
end

function MineAdjacent:run(ai, entity, args)
   local mining_zone = args.mining_zone
   local mining_zone_component = args.mining_zone:add_component('stonehearth:mining_zone')
   local zone_location = radiant.entities.get_world_grid_location(mining_zone)
   -- the actual location of the worker, which typically stops short of moving completely to the adjacent
   local worker_location = radiant.entities.get_world_grid_location(entity)
   local current_block = args.point_of_interest

   ai:unprotect_argument(args.mining_zone)

   while current_block do
      local reserved_blocks = stonehearth.mining:get_reserved_region_for_poi(current_block, self._adjacent_location, mining_zone)
      reserved_blocks:translate(-zone_location)
      self:_reserve_blocks(reserved_blocks)

      radiant.entities.turn_to_face(entity, current_block)
      ai:execute('stonehearth:run_effect', { effect = 'mine' })

      if not mining_zone:is_valid() then
         break
      end

      -- any time we yield, check to make sure we're still in the same location
      if radiant.entities.get_world_grid_location(entity) ~= worker_location then
         ai:abort('entity moved and must again think before mining more blocks')
         return
      end

      local loot = mining_zone_component:mine_point(current_block)
      local items = radiant.entities.spawn_items(loot, worker_location, 1, 3, { owner = entity })

      -- for the autotest
      radiant.events.trigger(entity, 'stonehearth:mined_location', { location = current_block })

      if not mining_zone:is_valid() then
         break
      end

      -- The reserved region may include support blocks. We must release it before looking
      -- for the next block to mine so that they will be included as candidates.
      self:_release_blocks()
      
      current_block = stonehearth.mining:resolve_point_of_interest(self._adjacent_location, mining_zone)
      -- prevent the worker from mining the block he is currently standing on
      if current_block and current_block == worker_location - Point3.unit_y then
         current_block = nil
      end
   end
end

function MineAdjacent:stop(ai, entity, args)
   self:_release_blocks()

   self._adjacent_location = nil
   self._destination_component = nil
end

function MineAdjacent:_reserve_blocks(region)
   self._reserved_blocks = region
   self._destination_component:get_reserved():modify(function(cursor)
         cursor:add_region(region)
      end)
end

function MineAdjacent:_release_blocks()
   if self._reserved_blocks and self._destination_component:is_valid() then
      self._destination_component:get_reserved():modify(function(cursor)
            cursor:subtract_region(self._reserved_blocks)
         end)
   end

   self._reserved_blocks = nil
end

return MineAdjacent
