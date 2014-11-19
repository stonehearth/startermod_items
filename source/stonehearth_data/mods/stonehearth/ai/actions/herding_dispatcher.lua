local constants = require 'constants'

local HerdingDispatcher = class()
HerdingDispatcher.name = 'herding dispatcher'
HerdingDispatcher.does = 'stonehearth:work'
HerdingDispatcher.args = {}
HerdingDispatcher.version = 2
HerdingDispatcher.priority = constants.priorities.work.HERDING

local ai = stonehearth.ai
return ai:create_compound_action(HerdingDispatcher)
         :execute('stonehearth:herding')
