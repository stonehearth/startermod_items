local Entity = _radiant.om.Entity
local Mine = class()

Mine.name = 'mine'
Mine.status_text = 'mining'
Mine.does = 'stonehearth:mining:mine'
Mine.args = {
   mining_zone = Entity
}
Mine.version = 2
Mine.priority = 1

local resource_radius = 0.5 -- distance from the center of a voxel to the edge
local entity_reach = 1.0   -- read this from entity_data.stonehearth:entity_reach
local tool_reach = 0.75     -- read this from entity_data.stonehearth:weapon_data.reach
local harvest_range = entity_reach + tool_reach + resource_radius

local ai = stonehearth.ai
return ai:create_compound_action(Mine)
         :execute('stonehearth:drop_carrying_now')
         :execute('stonehearth:goto_entity', {
            entity = ai.ARGS.mining_zone,
            stop_distance = harvest_range,
         })
         :execute('stonehearth:mining:get_block_to_mine', {
            mining_zone = ai.ARGS.mining_zone,
         })
         :execute('stonehearth:reserve_entity_destination_region', {
            entity = ai.ARGS.mining_zone,
            region = ai.PREV.reserved_region,
         })
         :execute('stonehearth:mining:mine_adjacent', {
            mining_zone = ai.ARGS.mining_zone,
            point_of_interest = ai.BACK(2).block,
         })
