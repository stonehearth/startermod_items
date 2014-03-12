local EatOnGround = class()
EatOnGround.name = 'eat on the ground'
EatOnGround.does = 'stonehearth:eat'
EatOnGround.args = {}
EatOnGround.version = 2
EatOnGround.priority = 1

local ai = stonehearth.ai
return ai:create_compound_action(EatOnGround)
            :execute('stonehearth:wait_for_attribute_above', { attribute = 'hunger', value = 120 })
            :execute('stonehearth:get_food')
            :execute('stonehearth:wander', { radius = 5, radius_min = 3 })
            :execute('stonehearth:sit_on_ground')
            :execute('stonehearth:eat_carrying')
