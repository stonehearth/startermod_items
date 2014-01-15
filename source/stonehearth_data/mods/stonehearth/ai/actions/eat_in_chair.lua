local event_service = stonehearth.events

local EatCarryingInChair = class()
EatCarryingInChair.name = 'eat in a chair, like a human being'
EatCarryingInChair.does = 'stonehearth:eat'
EatCarryingInChair.args = {}
EatCarryingInChair.version = 2
EatCarryingInChair.priority = 1

local ai = stonehearth.ai
return ai.create_compound_action(EatCarryingInChair)
            :execute('stonehearth:wait_for_attribute_above', { attribute = 'hunger', value = 80 })
            :execute("stonehearth:pickup_item_made_of", { material = 'food'}})
            :execute('stonehearth:sit_in_any_chair')
            :execute('stonehearth:eat_carrying')
