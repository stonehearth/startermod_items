local DropCarryingIfStacksFull = class()

DropCarryingIfStacksFull.name = 'drop if stacks full'
DropCarryingIfStacksFull.does = 'stonehearth:drop_carrying_if_stacks_full'
DropCarryingIfStacksFull.args = { }
DropCarryingIfStacksFull.version = 2
DropCarryingIfStacksFull.priority = 1

function DropCarryingIfStacksFull:start_thinking(ai, entity, args)
   if ai.CURRENT.carrying then
      local item = ai.CURRENT.carrying:get_component('item')
      if item then
         if item:get_stacks() == item:get_max_stacks() then
            ai.CURRENT.carrying = nil
         end
      end
   end
   ai:set_think_output()
end

function DropCarryingIfStacksFull:run(ai, entity, args)
   local carrying = radiant.entities.get_carrying(entity)
   if carrying then
      local item = carrying:get_component('item')
      if item then
         if item:get_stacks() == item:get_max_stacks() then
            ai:execute('stonehearth:wander', { radius_min = 1, radius = 3 })
            ai:execute('stonehearth:drop_carrying_now', {})
         end
      end
   end
end

return DropCarryingIfStacksFull
