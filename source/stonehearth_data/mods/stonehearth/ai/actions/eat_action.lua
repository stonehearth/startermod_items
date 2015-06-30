local constants = require 'constants'

local Eat = class()
Eat.name = 'eat to live'
Eat.status_text = 'getting food' -- eat item will override this when it runs
Eat.does = 'stonehearth:eat'
Eat.args = { }
Eat.version = 2
Eat.priority = 1

local ai = stonehearth.ai
return ai:create_compound_action(Eat)
         :execute('stonehearth:get_food')
         :execute('stonehearth:find_seat_and_eat')
