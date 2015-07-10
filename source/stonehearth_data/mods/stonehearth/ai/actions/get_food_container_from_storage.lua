local Entity = _radiant.om.Entity
local GetFoodFromContainerFromStorage = class()

GetFoodFromContainerFromStorage.name = 'get food from container in storage'
GetFoodFromContainerFromStorage.does = 'stonehearth:get_food'
GetFoodFromContainerFromStorage.args = { }
GetFoodFromContainerFromStorage.version = 2
GetFoodFromContainerFromStorage.priority = 1

-- Note: we don't use pickup_item_type here because then AIs could end up pointlessly picking up
-- food containers that are out in the open, just to put them down again and eat.
local ai = stonehearth.ai
return ai:create_compound_action(GetFoodFromContainerFromStorage)
         :execute('stonehearth:drop_carrying_now', {})
         :execute('stonehearth:material_to_filter_fn', { material = 'food_container' })
         :execute('stonehearth:find_path_to_storage_containing_entity_type', { 
         	filter_fn = ai.PREV.filter_fn,
         	description = 'find path to food container', })
         :execute('stonehearth:pickup_item_type_from_storage', {
            filter_fn = ai.BACK(2).filter_fn,
            storage = ai.PREV.destination,
            description = 'find path to food container',
         })
         :execute('stonehearth:reserve_entity', { entity = ai.PREV.item })
         :execute('stonehearth:drop_carrying_now', {})
         :execute('stonehearth:get_food_from_container_adjacent', { container = ai.BACK(3).item })
