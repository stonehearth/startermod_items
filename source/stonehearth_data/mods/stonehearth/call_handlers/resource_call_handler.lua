local priorities = require('constants').priorities.worker_task

local ResourceCallHandler = class()

--- Remote entry point for requesting that a tree get harvested
-- @param tree The entity which you would like chopped down
-- @return true on success, false on failure

local all_harvest_tasks = {}

function ResourceCallHandler:harvest_tree(session, response, tree)
   if not tree or not tree:is_valid() then
      return false
   end

   local id = tree:get_id()
   if not all_harvest_tasks[id] then
      all_harvest_tasks[id] = stonehearth.tasks:get_scheduler('stonehearth:workers', session.faction)
                                :create_task('stonehearth:chop_tree', { tree = tree }) 
                                :set_name('chop tree task')
                                :once()
                                :start()
      
      local eff_mgr_component = tree:get_component('stonehearth:effect_manager')
      if eff_mgr_component then
         eff_mgr_component:show_effect('harvest_tree')
      end
   end

   return true
end

function ResourceCallHandler:harvest_plant(session, response, plant)
   stonehearth.tasks:get_scheduler('stonehearth:workers', session.faction)
                              :create_task('stonehearth:harvest_plant', { plant = plant })
                              :set_name('harvest plant task')
                              :once()
                              :start()

   local eff_mgr_component = plant:get_component('stonehearth:effect_manager')
   if eff_mgr_component then
      eff_mgr_component:show_effect('harvest_plant')
   end

   return true
end

function ResourceCallHandler:shear_sheep(session, response, sheep)
   local id = sheep:get_id()
   if not all_harvest_tasks [id] then
      local harvest_task = worker_scheduler:add_worker_task('shear_sheep')
                     :set_worker_filter_fn(not_carrying_fn)
                     :add_work_object(sheep)
                     :set_action('stonehearth:shear_sheep')
                     :set_max_workers(1)
                     :set_priority(priorities.GATHER_RESOURCE)

   harvest_task:set_action_fn(
      function (path)
         return 'stonehearth:harvest_plant', path, harvest_task
      end
   )

   harvest_task:start()

      --radiant.effects.run_effect(plant, '/stonehearth/data/effects/harvest_berries_overlay_effect')
   end

   return true
end

return ResourceCallHandler
