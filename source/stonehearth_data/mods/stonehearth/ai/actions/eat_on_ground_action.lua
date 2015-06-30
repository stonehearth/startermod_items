local EatOnGround = class()
EatOnGround.name = 'eat on the ground'
EatOnGround.does = 'stonehearth:find_seat_and_eat'
EatOnGround.args = {}
EatOnGround.version = 2
EatOnGround.priority = 1

local ai = stonehearth.ai
return ai:create_compound_action(EatOnGround)
            :execute('stonehearth:wander', { radius = 5, radius_min = 3 })
            :execute('stonehearth:sit_on_ground')
            :execute('stonehearth:eat_carrying')
