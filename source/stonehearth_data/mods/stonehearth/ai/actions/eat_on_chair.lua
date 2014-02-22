local event_service = stonehearth.events

local EatOnChair = class()
EatOnChair.name = 'eat on a chair, like a human being'
EatOnChair.does = 'stonehearth:eat'
EatOnChair.args = {}
EatOnChair.version = 2
EatOnChair.priority = 2

local ai = stonehearth.ai
return ai:create_compound_action(EatOnChair)
            :execute('stonehearth:wait_for_attribute_above', { attribute = 'hunger', value = 80 })
            :execute("stonehearth:get_food")
            :execute('stonehearth:sit_on_chair')
            :execute('stonehearth:eat_carrying')
