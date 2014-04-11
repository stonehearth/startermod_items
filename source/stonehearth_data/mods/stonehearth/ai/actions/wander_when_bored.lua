local WanderWhenBored = class()

WanderWhenBored.name = 'wander when bored'
WanderWhenBored.does = 'stonehearth:idle:bored'
WanderWhenBored.args = { }
WanderWhenBored.version = 2
WanderWhenBored.priority = 1

local ai = stonehearth.ai
return ai:create_compound_action(WanderWhenBored)
         :execute('stonehearth:wander_within_leash', { radius = 5, radius_min = 2 })
