local Entity = _radiant.om.Entity
local Point3 = _radiant.csg.Point3
local Region3 = _radiant.csg.Region3
local GetBlockToMine = class()
local log = radiant.log.create_logger('mining')

GetBlockToMine.name = 'get block to mine'
GetBlockToMine.does = 'stonehearth:mining:get_block_to_mine'
GetBlockToMine.args = {
   mining_zone = Entity,
}
GetBlockToMine.think_output = {
   block = Point3,
   reserved_region = Region3,
}
GetBlockToMine.version = 2
GetBlockToMine.priority = 1

function GetBlockToMine:start_thinking(ai, entity, args)
   local from = ai.CURRENT.location
   local mining_zone = args.mining_zone

   local block = stonehearth.mining:resolve_point_of_interest(from, mining_zone)

   if block then
      local reserved_region = stonehearth.mining:get_reserved_region_for_poi(block, from, mining_zone)
      ai:set_think_output({
         block = block,
         reserved_region = reserved_region,
      })
   else
      -- will wait for something to trigger a restart thinking on the greater compound action
      log:warning('Could not find point to mine')
   end
end

return GetBlockToMine
