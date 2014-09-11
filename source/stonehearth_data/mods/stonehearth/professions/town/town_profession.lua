local town_class = {}

function town_class.promote(entity, json)
   
end

function town_class.demote(entity)

end

function town_class.level_up(entity)
   local town = entity
   local profession_component = town:get_component('stonehearth:profession')

   -- xx, send a notification. town level up!

   -- add a citizen
end


return town_class
