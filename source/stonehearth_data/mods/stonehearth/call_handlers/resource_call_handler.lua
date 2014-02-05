local priorities = require('constants').priorities.worker_task

local ResourceCallHandler = class()

--- Remote entry point for requesting that a tree get harvested
-- @param tree The entity which you would like chopped down
-- @return true on success, false on failure

local all_harvest_tasks = {}

function ResourceCallHandler:_harvest_node(resource_node, component_name, task_name, task_args)
   if not resource_node or not resource_node:is_valid() then
      return false
   end

   local id = resource_node:get_id()
   if not all_harvest_tasks[id] then
      local node_component = resource_node:get_component(component_name)
      if node_component then
         all_harvest_tasks[id] = stonehearth.tasks:get_scheduler('stonehearth:workers', session.faction)
                                   :create_task(task_name, task_args) 
                                   :add_entity_effect(resource_node, node:get_harvest_overlay_effect())
                                   :once()
                                   :start()
      end
   end
   return true
end

function ResourceCallHandler:harvest_tree(session, response, tree)
   if not tree or not tree:is_valid() then
      return false
   end

   local id = tree:get_id()
   if not all_harvest_tasks[id] then
      local node_component = tree:get_component('stonehearth:resource_node')
      if node_component then
         local effect_name = node_component:get_harvest_overlay_effect()
         all_harvest_tasks[id] = stonehearth.tasks:get_scheduler('stonehearth:workers', session.faction)
                                   :create_task('stonehearth:chop_tree', { tree = tree })
                                   :add_entity_effect(tree, effect_name)
                                   :once()
                                   :start()
      end
   end
   return true
end

function ResourceCallHandler:harvest_plant(session, response, plant)
   if not plant or not plant:is_valid() then
      return false
   end

   local id = plant:get_id()
   if not all_harvest_tasks[id] then
      local node_component = plant:get_component('stonehearth:renewable_resource_node')
      if node_component then
         local effect_name = node_component:get_harvest_overlay_effect()
         all_harvest_tasks[id] = stonehearth.tasks:get_scheduler('stonehearth:workers', session.faction)
                                   :create_task('stonehearth:harvest_plant', { plant = plant })
                                   :add_entity_effect(plant, effect_name)
                                   :once()
                                   :start()

         radiant.events.listen(plant, 'stonehearth:is_harvestable', function(e) 
               local plant = e.entity
               if not plant or not plant:is_valid() then
                  return radiant.events.UNLISTEN
               end
               all_harvest_tasks[plant:get_id()] = nil
            end)
      end
   end
   return true
end

function ResourceCallHandler:shear_sheep(session, response, sheep)
   asset(false, 'shear_sheep was broken, so i deleted it =)  -tony')
end

return ResourceCallHandler
