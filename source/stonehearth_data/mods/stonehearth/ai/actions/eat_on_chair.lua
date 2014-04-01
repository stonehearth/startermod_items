local EatOnChair = class()
EatOnChair.name = 'eat on a chair'
EatOnChair.does = 'stonehearth:find_seat_and_eat'
EatOnChair.args = {}
EatOnChair.version = 2
EatOnChair.priority = 2

local ai = stonehearth.ai
return ai:create_compound_action(EatOnChair)
            :execute('stonehearth:sit_on_chair')
            :execute('stonehearth:eat_carrying')
