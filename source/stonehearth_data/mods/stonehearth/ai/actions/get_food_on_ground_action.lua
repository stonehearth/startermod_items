local Entity = _radiant.om.Entity
local GetFoodOnGround = class()

GetFoodOnGround.name = 'get food on the ground'
GetFoodOnGround.does = 'stonehearth:get_food'
GetFoodOnGround.args = { }
GetFoodOnGround.version = 2
GetFoodOnGround.priority = 1
GetFoodOnGround.think_output = {
   item = Entity,   -- item that was picked up
}

local ai = stonehearth.ai
return ai:create_compound_action(GetFoodOnGround)
         :execute('stonehearth:pickup_item_made_of', { material = "food" })
         :set_think_output({ item = ai.PREV.item })
