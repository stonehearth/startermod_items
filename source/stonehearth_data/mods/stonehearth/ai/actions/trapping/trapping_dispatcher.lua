local constants = require 'constants'

local TrappingDispatcher = class()
TrappingDispatcher.name = 'trapping dispatcher'
TrappingDispatcher.does = 'stonehearth:work'
TrappingDispatcher.args = {}
TrappingDispatcher.version = 2
TrappingDispatcher.priority = constants.priorities.work.TRAPPING

local ai = stonehearth.ai
return ai:create_compound_action(TrappingDispatcher)
         :execute('stonehearth:trapping')
