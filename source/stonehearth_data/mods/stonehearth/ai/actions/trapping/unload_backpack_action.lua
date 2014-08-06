local UnloadBackpackDispatcher = class()

UnloadBackpackDispatcher.name = 'unload backpack'
UnloadBackpackDispatcher.does = 'stonehearth:trapping:unload_backpack'
UnloadBackpackDispatcher.args = {}
UnloadBackpackDispatcher.version = 2
UnloadBackpackDispatcher.priority = 2

local ai = stonehearth.ai
return ai:create_compound_action(UnloadBackpackDispatcher)
         :execute('stonehearth:restock_items_in_backpack')
