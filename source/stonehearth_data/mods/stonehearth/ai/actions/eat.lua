local constants = require('constants')

local Eat = class()
Eat.name = 'eat to live'
Eat.does = 'stonehearth:top'
Eat.args = { }
Eat.version = 2
Eat.priority = constants.priorities.needs.EAT

local ai = stonehearth.ai
return ai:create_compound_action(Eat)
         :execute('stonehearth:eat')
