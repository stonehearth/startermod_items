local FollowShepherd = class()

FollowShepherd.name = 'follow shepherd'
FollowShepherd.does = 'stonehearth:top'
FollowShepherd.args = {}
FollowShepherd.version = 2
FollowShepherd.priority = 2

--This action is from the pasture tag, and it runs only when the 
--shepherded animal component says to do so. 
function FollowShepherd:start_thinking(ai, entity, args)
   local equipment_component = entity:get_component('stonehearth:equipment')
   if not equipment_component then
      return
   end
   local pasture_tag = equipment_component:has_item_type('stonehearth:pasture_tag')
   if not pasture_tag then
      return
   end
   local shepherded_component = pasture_tag:get_component('stonehearth:shepherded_animal')
   if not shepherded_component then
      return
   end
   if shepherded_component:get_following() then
      self._target_shepherd = shepherded_component:get_last_shepherd()
      ai:set_think_output()
   end
end

function FollowShepherd:run(ai, entity, args)
   if self._target_shepherd then
      ai:execute('stonehearth:follow_entity', {
         target = self._target_shepherd})
   end
end

return FollowShepherd