local constants = require 'constants'

local CraftingTaskDispatcher = class()
CraftingTaskDispatcher.name = 'do crafting'
CraftingTaskDispatcher.does = 'stonehearth:work'
CraftingTaskDispatcher.args = {}
CraftingTaskDispatcher.version = 2
CraftingTaskDispatcher.priority = constants.priorities.work.CRAFTING

-- Dispatches the most basic unskilled tasks, usually the domain of workers. 
-- Things like: hauling, building, picking berries, lighting fires, etc

local ai = stonehearth.ai
return ai:create_compound_action(CraftingTaskDispatcher)
         :execute('stonehearth:crafting')
