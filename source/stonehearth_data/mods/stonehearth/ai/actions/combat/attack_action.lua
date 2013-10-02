local Attack = class()

Attack.name = 'Attack!'
Attack.does = 'stonehearth.attack'
Attack.priority = 0

function Attack:run(ai, entity, target)
   assert(target)

   while true do
      if radiant.entities.distance_between(entity, target) < 3 then
         -- in Attack range
         ai:execute('stonehearth.attack.melee', target)
      else 
         ai:execute('stonehearth.attack.chase_target', target)
      end
   end
   
end

return Attack