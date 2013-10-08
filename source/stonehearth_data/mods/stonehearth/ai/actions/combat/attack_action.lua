local Attack = class()

Attack.name = 'Attack!'
Attack.does = 'stonehearth:attack'
Attack.priority = 0

function Attack:__init(ai, entity)
   self._entity = entity
end

function Attack:run(ai, entity, target)
   assert(target)

   while true do
      if radiant.entities.distance_between(entity, target) < 6 then
         ai:execute('stonehearth:attack:melee', target)
      else 
         ai:execute('stonehearth:attack:chase_target', target)
      end
   end
end

function Attack:get_melee_weapon()
   return self._entity:add_component('stonehearth:equipment'):get_melee_weapon()
end 

return Attack