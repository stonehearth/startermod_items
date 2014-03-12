local PetEatFromContainerAction = class()

PetEatFromContainerAction.name = 'pet eat from container'
PetEatFromContainerAction.does = 'stonehearth:eat'
PetEatFromContainerAction.args = {}
PetEatFromContainerAction.version = 2
PetEatFromContainerAction.priority = 1

local ai = stonehearth.ai
return ai:create_compound_action(PetEatFromContainerAction)
   :execute('stonehearth:wait_for_attribute_above', { attribute = 'hunger', value = 100 })
   :execute('stonehearth:goto_item_made_of', { material = 'food_container' })
   :execute('stonehearth:eat_item', { item = ai.PREV.item })
