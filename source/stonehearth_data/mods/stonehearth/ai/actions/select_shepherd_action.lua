local SelectShepherd = class()
local Entity = _radiant.om.Entity

SelectShepherd.name = 'select shepherd'
SelectShepherd.does = 'stonehearth:select_shepherd'
SelectShepherd.args = {}
SelectShepherd.think_output = {
   shepherd = Entity   --the entity to follow
}
SelectShepherd.version = 2
SelectShepherd.priority = 1

function SelectShepherd:start_thinking(ai, entity, args)
   local equipment_component = entity:get_component('stonehearth:equipment')
   local pasture_tag = equipment_component:has_item_type('stonehearth:pasture_tag')
   local shepherded_component = pasture_tag:get_component('stonehearth:shepherded_animal')
   local args = {
      should_follow = shepherded_component:get_following(),
      shepherd =  shepherded_component:get_last_shepherd()
   }
   self._ai = ai
   self._follow_listener = radiant.events.listen(entity, 'stonehearth:shepherded_animal_follow_status_change', self, self._on_follow_changed)
   self:_on_follow_changed(args)
end

function SelectShepherd:_on_follow_changed(args)
   if args.should_follow then
      self._ai:set_think_output({
         shepherd = args.shepherd
         })
   end
end

function SelectShepherd:stop_thinking(ai, entity, args)
   if self._follow_listener then
      self._follow_listener:destroy()
      self._follow_listener = nil
   end
end

return SelectShepherd