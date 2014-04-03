local ClearWorkshop = class()

function ClearWorkshop:run(thread, args)
   local workshop = args.workshop
   local outbox = workshop:get_component('stonehearth:workshop')
                          :get_outbox()
                          :get_component('stonehearth:stockpile')                          
   local task_group = args.task_group
   
   local container = workshop:get_component('entity_container')
   while container:num_children() > 0 do
      local id, item = container:first_child()
      local args = {
         outbox = outbox,
         item = item,
      }
      task_group:create_task('stonehearth:move_item_to_outbox', args)
                      :set_priority(stonehearth.constants.priorities.top.WORK)
                      :once()
                      :start()
                      :wait()
   end
end

return ClearWorkshop
