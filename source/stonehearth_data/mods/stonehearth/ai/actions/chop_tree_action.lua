local event_service = require 'services.event.event_service'
local personality_service = require 'services.personality.personality_service'

local ChopTreeAction = class()

ChopTreeAction.does = 'stonehearth:chop_tree'
ChopTreeAction.priority = 1

function ChopTreeAction:__init()
   --Load the activity log if it's never been loaded before
   personality_service:load_activity('stonehearth:personal_logs:chopping_wood')
end

function ChopTreeAction:run(ai, entity, path)
   local tree = path:get_destination()

   -- the tree could be nil here if something happened to it before
   -- we got a chance to chop it down.  nothing terribly bad would happen
   -- if we continued from here... the coroutine would crash and the
   -- entity would find something else to do.  however, this is a fairly
   -- common occurance, so just abort gracefully instead.
   if not tree then
      ai:abort('tree is nil in stonehearth:chop_tree') 
   end

   --xxx localize   
   local worker_name = radiant.entities.get_display_name(entity)
   local tree_name = radiant.entities.get_display_name(tree)
   event_service:add_entry(worker_name .. ' is harvesting ' .. tree_name)

   ai:execute('stonehearth:follow_path', path)
   radiant.entities.turn_to_face(entity, tree)
   ai:execute('stonehearth:run_effect', 'chop')
   
   local factory = tree:get_component('stonehearth:resource_node')
   if factory then
      local location = radiant.entities.get_world_grid_location(entity)
      factory:spawn_resource(location)
   end
   
   --Log in personal event log
   entity:get_component('stonehearth:personality'):register_notable_event('chopping_logs', 50)

end

return ChopTreeAction

