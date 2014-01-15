local event_service = stonehearth.events

local EatCarryingOnGround = class()
EatCarryingOnGround.name = 'eat on the ground'
EatCarryingOnGround.does = 'stonehearth:eat'
EatCarryingOnGround.args = {}
EatCarryingOnGround.version = 2
EatCarryingOnGround.priority = 1

local ai = stonehearth.ai
return ai.create_compound_action(EatCarryingOnGround)
            :execute('stonehearth:wait_for_attribute_above', { attribute = 'hunger', value = 120 })
            :execute("stonehearth:pickup_item_made_of", { material = 'food'}})
            :execute('stonehearth:choose_point_around_leash', { radius = 5 })
            :execute('stonehearth:goto_location', { location = ai.PREV.location })
            :execute('stonehearth:sit_on_ground')
            :execute('stonehearth:eat_carrying')
