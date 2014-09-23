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

local ai = stonehearth.ai
return ai:create_compound_action(Mine)
         :execute('stonehearth:drop_carrying_now')
         :execute('stonehearth:goto_entity', {
            entity = ai.ARGS.mining_zone,
         })
         :execute('stonehearth:reserve_entity_destination', {
            entity = ai.ARGS.mining_zone,
            location = ai.PREV.point_of_interest
         })
         :execute('stonehearth:mining:mine_adjacent', {
            mining_zone = ai.ARGS.mining_zone,
            point_of_interest = ai.BACK(2).point_of_interest,
         })
