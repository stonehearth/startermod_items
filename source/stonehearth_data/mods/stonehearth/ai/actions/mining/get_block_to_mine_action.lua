local Entity = _radiant.om.Entity
local Point3 = _radiant.csg.Point3
local GetBlockToMine = class()
local log = radiant.log.create_logger('mining')

GetBlockToMine.name = 'get block to mine'
GetBlockToMine.does = 'stonehearth:mining:get_block_to_mine'
GetBlockToMine.args = {
   mining_zone = Entity,
   default_point_of_interest = Point3,
}
GetBlockToMine.think_output = {
   block = Point3
}
GetBlockToMine.version = 2
GetBlockToMine.priority = 1

function GetBlockToMine:start_thinking(ai, entity, args)
   local block = stonehearth.mining:resolve_point_of_interest(
                 ai.CURRENT.location, args.mining_zone, args.default_point_of_interest)

   if block then
      ai:set_think_output({ block = block })
   else
      -- will wait for something to trigger a restart thinking on the greater compound action
      log:warning('Could not find point to mine')
   end
end

return GetBlockToMine
