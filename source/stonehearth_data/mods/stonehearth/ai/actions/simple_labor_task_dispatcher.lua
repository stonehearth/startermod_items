local constants = require 'constants'

local SimpleLaborDispatcher = class()
SimpleLaborDispatcher.name = 'do simple labor'
SimpleLaborDispatcher.does = 'stonehearth:work'
SimpleLaborDispatcher.args = {}
SimpleLaborDispatcher.version = 2
SimpleLaborDispatcher.priority = constants.priorities.work.SIMPLE_LABOR

-- Dispatches the most basic unskilled tasks, usually the domain of workers. 
-- Things like: hauling, building, picking berries, lighting fires, etc

local ai = stonehearth.ai
return ai:create_compound_action(SimpleLaborDispatcher)
         :execute('stonehearth:simple_labor')
