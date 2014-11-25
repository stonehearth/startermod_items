local ClearWorkshop = class()

function ClearWorkshop:run(thread, args)
   local workshop = args.workshop
   local task_group = args.task_group
   local crafter = args.crafter
   
   local container = workshop:get_component('entity_container')


   while container and container:num_children() > 0 do
      local id, child = container:first_child()
      if not child or not child:is_valid() then
         container:remove_child(id)
      else
         local args = {
            item = child,
         }

         local task = task_group:create_task('stonehearth:clear_workshop', args)
                         :set_priority(stonehearth.constants.priorities.work.CRAFTING)
                         :once()
                         :start()

         local check_task_fn = function()
            if not child:is_valid() or not container:get_child(child:get_id()) then
               task:destroy()
            end
         end

         local halt_listener = radiant.events.listen(crafter, 'stonehearth:ai:halt', check_task_fn)
         task:wait()
         halt_listener:destroy()
      end
   end
end


return ClearWorkshop
