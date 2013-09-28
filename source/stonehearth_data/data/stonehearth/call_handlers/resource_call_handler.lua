
local ResourceCallHandler = class()

--- Remote entry point for requesting that a tree get harvested
-- @param tree The entity which you would like chopped down
-- @return true on success, false on failure

function ResourceCallHandler:harvest_tree(session, response, tree)
   local worker_scheduler = radiant.mods.load('stonehearth').get_service('worker_scheduler'):get_worker_scheduler(session.faction)

   -- Any worker that's not carrying anything will do...
   local not_carrying_fn = function (worker)
      return radiant.entities.get_carrying(worker) == nil
   end

   worker_scheduler:add_worker_task('chop_tree')
                  :set_worker_filter_fn(not_carrying_fn)
                  :add_work_object(tree)
                  :set_action('stonehearth.chop_tree')
                  :start()
   return true
end

return ResourceCallHandler
