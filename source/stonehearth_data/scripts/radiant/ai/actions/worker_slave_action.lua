local SlaveAction  = class()

local md = require 'radiant.core.md'
local om = require 'radiant.core.om'
local ai_mgr = require 'radiant.ai.ai_mgr'

ai_mgr:register_action('radiant.actions.worker_scheduler_slave', SlaveAction)

function SlaveAction:run(ai, entity, scheduler, action, path, ...)
   ai:execute('radiant.actions.follow_path', path)

   if action == 'chop' then
      local node = ...
      ai:execute('radiant.actions.chop_tree', node)
   elseif action == 'pickup' then
      local item = ...
      ai:execute('radiant.actions.pickup_item', item)
   elseif action == 'restock' then
      local stockpile, location = ...
      ai:execute('radiant.actions.drop_carrying', location)
   elseif action == 'construct' then
      local structure, block = ...
      ai:execute('radiant.actions.construct', structure, block)
   elseif action == 'teardown' then
      local structure, block = ...
      ai:execute('radiant.actions.tear_down', structure, block)
   end
   scheduler:finish_activity(entity)
end
