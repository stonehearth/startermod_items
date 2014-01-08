local priorities = require('constants').priorities.worker_task

local ResourceCallHandler = class()

--- Remote entry point for requesting that a tree get harvested
-- @param tree The entity which you would like chopped down
-- @return true on success, false on failure

local all_harvest_tasks = {}

function ResourceCallHandler:harvest_tree(session, response, tree)
   local worker_scheduler = stonehearth.worker_scheduler:get_worker_scheduler(session.faction)

   -- Any worker that's not carrying anything will do...
   local not_carrying_fn = function (worker)
      return radiant.entities.get_carrying(worker) == nil
   end

   local id = tree:get_id()
   if not all_harvest_tasks[id] then
      --[[
      all_harvest_tasks[id] = worker_scheduler:add_worker_task('chop_tree')
                                                 :set_worker_filter_fn(not_carrying_fn)
                                                 :add_work_object(tree)
                                                 :set_action('stonehearth:chop_tree')
                                                 :set_max_workers(1)
                                                 :set_priority(priorities.CHOP_TREE)
                                                 :start()
      ]]
      all_harvest_tasks[id] = stonehearth.tasks:get_scheduler('stonehearth:workers')
                                :create_task('stonehearth:chop_tree', { tree = tree })
                                :set_name('chop tree task')
                                :once()
                                :start()

      radiant.effects.run_effect(tree, '/stonehearth/data/effects/chop_overlay_effect')
   end
   
   return true
end

function ResourceCallHandler:harvest_plant(session, response, plant)
   local worker_scheduler = stonehearth.worker_scheduler:get_worker_scheduler(session.faction)

   -- Any worker that's not carrying anything will do...
   local not_carrying_fn = function (worker)
      return radiant.entities.get_carrying(worker) == nil
   end

   local harvest_task = worker_scheduler:add_worker_task('harvest_berries')
                   :set_worker_filter_fn(not_carrying_fn)
                   :add_work_object(plant)
                   :set_priority(priorities.GATHER_FOOD)

   harvest_task:set_action_fn(
      function (path)
         return 'stonehearth:harvest_plant', path, harvest_task
      end
   )

   harvest_task:start()

   return true
end

function ResourceCallHandler:shear_sheep(session, response, sheep)
   local worker_scheduler = stonehearth.worker_scheduler:get_worker_scheduler(session.faction)

   -- Any worker that's not carrying anything will do...
   local not_carrying_fn = function (worker)
      return radiant.entities.get_carrying(worker) == nil
   end

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
