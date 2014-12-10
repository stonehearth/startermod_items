local WaitForTrailingAnimal = class()
local Entity = _radiant.om.Entity
local Point3 = _radiant.csg.Point3

WaitForTrailingAnimal.name = 'wait for trailing animal'
WaitForTrailingAnimal.does = 'stonehearth:wait_for_trailing_animal'
WaitForTrailingAnimal.args = {}
WaitForTrailingAnimal.think_output = {
   animal = Entity, 
   pasture_center = Point3
}
WaitForTrailingAnimal.version = 2
WaitForTrailingAnimal.priority = 1

function WaitForTrailingAnimal:start_thinking(ai, entity, args)
   self._ai = ai
   self._add_animal_listener = radiant.events.listen(entity, 'stonehearth:add_trailing_animal', self, self._on_animal_added)
   
   --If there are any animals currenty trailing us, pick one to return to its pasture
   local shepherd_class = entity:get_component('stonehearth:job'):get_curr_job_controller()
   if shepherd_class and shepherd_class.get_trailing_animals then
      local trailed_animals, num_trailed = shepherd_class:get_trailing_animals()
      if num_trailed and num_trailed > 0 then
         local target_sheep
         --Get a sheep (TODO: do we need to make sure it's not the same sheep?)
         for id, sheep in pairs(trailed_animals) do
            target_sheep = sheep
            break
         end
         --get it's pasture
         local equipment_component = target_sheep:add_component('stonehearth:equipment')
         local pasture_collar = equipment_component:has_item_type('stonehearth:pasture_tag')
         local shepherded_animal_component = pasture_collar:get_component('stonehearth:shepherded_animal')

         --Call the function
         local args = {
            animal = target_sheep, 
            pasture = shepherded_animal_component:get_pasture()
         }
         self:_on_animal_added(args)
      end
   end

end

--TODO: test what happens when a sheep is eaten AS the shepherd leads him home
function WaitForTrailingAnimal:_on_animal_added(args)
   if args.animal and args.pasture then
      self._ai:set_think_output({
         animal = args.animal, 
         pasture_center = self:_get_pasture_center(args.pasture)
         })
   end
end

function WaitForTrailingAnimal:_get_pasture_center(pasture_entity)
   local pasture_component = pasture_entity:get_component('stonehearth:shepherd_pasture')
   return pasture_component:get_center_point()
end


function WaitForTrailingAnimal:stop_thinking(ai, entity, args)
   if self._add_animal_listener then
      self._add_animal_listener:destroy()
      self._add_animal_listener = nil
   end
end

return WaitForTrailingAnimal