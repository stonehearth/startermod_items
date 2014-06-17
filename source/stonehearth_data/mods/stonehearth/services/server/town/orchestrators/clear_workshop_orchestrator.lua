local ClearWorkshop = class()

function ClearWorkshop:run(thread, args)
   local workshop = args.workshop
   local outbox = workshop:get_component('stonehearth:workshop')
                          :get_outbox()
                          :get_component('stonehearth:stockpile')                          
   local task_group = args.task_group
   local crafter = args.crafter
   
   local container = workshop:get_component('entity_container')


   while container:num_children() > 0 do
      local id, child = container:first_child()
      if not child or not child:is_valid() then
         container:remove_child(id)
      else
         local args = {
            outbox = outbox,
            item = child,
         }

         local task = task_group:create_task('stonehearth:move_item_to_outbox', args)
                         :set_priority(stonehearth.constants.priorities.top.WORK)
                         :once()
                         :start()


         local check_task_fn = function()
            if not child:is_valid() or not container:get_child(child:get_id()) then
               task:destroy()
            end
         end

         radiant.events.listen(crafter, 'stonehearth:ai:halt', check_task_fn)
         task:wait()
         radiant.events.unlisten(crafter, 'stonehearth:ai:halt', check_task_fn)
      end
   end
end


return ClearWorkshop
