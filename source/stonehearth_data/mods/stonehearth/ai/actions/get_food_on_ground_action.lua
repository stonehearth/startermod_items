local Entity = _radiant.om.Entity
local GetFoodOnGround = class()

GetFoodOnGround.name = 'get food on the ground'
GetFoodOnGround.does = 'stonehearth:get_food'
GetFoodOnGround.args = { }
GetFoodOnGround.version = 2
GetFoodOnGround.priority = 1

local ai = stonehearth.ai
return ai:create_compound_action(GetFoodOnGround)
         :execute('stonehearth:pickup_item_made_of', { material = "food" })

