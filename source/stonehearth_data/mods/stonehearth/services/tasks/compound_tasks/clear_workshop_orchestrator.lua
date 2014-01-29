local ClearWorkshop = class()

function ClearWorkshop:run(thread, args)
   local workshop = args.workshop
   local outbox = workshop:get_component('stonehearth:workshop')
                          :get_outbox()
                          :get_component('stonehearth:stockpile')
   local container = workshop:get_component('entity_container')
   while container:num_children() > 0 do
      local id, item = container:first_child()
      thread:execute('stonehearth:move_item_to_outbox', {
            outbox = outbox,
            item = item,
         })
   end
end

return ClearWorkshop
