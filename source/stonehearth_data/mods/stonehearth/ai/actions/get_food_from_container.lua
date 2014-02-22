local Entity = _radiant.om.Entity
local GetFoodFromContainer = class()

GetFoodFromContainer.name = 'get food from container'
GetFoodFromContainer.does = 'stonehearth:get_food'
GetFoodFromContainer.args = { }
GetFoodFromContainer.version = 2
GetFoodFromContainer.priority = 1

local ai = stonehearth.ai
return ai:create_compound_action(GetFoodFromContainer)
         :execute('stonehearth:drop_carrying_now', {})
         :execute('stonehearth:goto_item_made_of', { material = "food_container" })
         :execute('stonehearth:get_food_from_container_adjacent', { container = ai.PREV.item })

