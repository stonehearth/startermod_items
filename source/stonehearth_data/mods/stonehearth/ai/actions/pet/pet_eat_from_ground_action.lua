local PetEatOnGround = class()

PetEatOnGround.name = 'pet eat from ground'
PetEatOnGround.does = 'stonehearth:eat'
PetEatOnGround.args = {}
PetEatOnGround.version = 2
PetEatOnGround.priority = 2

local ai = stonehearth.ai
return ai:create_compound_action(PetEatOnGround)
   :execute('stonehearth:goto_item_made_of', { material = 'food' })
   -- reserve item after we get there. let humans reserve first
   :execute('stonehearth:reserve_entity', { entity = ai.PREV.item })
   :execute('stonehearth:eat_item', { item = ai.PREV.entity })
