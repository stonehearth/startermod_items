local NightlyWolfAttack = class()

local Point3 = _radiant.csg.Point3

function NightlyWolfAttack:__init()
   
   --radiant.events.listen('radiant:events:calendar:sunset', self)
end

function NightlyWolfAttack:load_faction()
   local mod = radiant.mods.load('stonehearth')
   self._civ_faction = mod.population:get_faction('stonehearth:factions:ascendancy')
end

NightlyWolfAttack['radiant:events:calendar:sunset'] = function(self, _)
   
   if not self._civ_faction then
      self:load_faction()
   end

   for i = 1, 1 do
      self:spawn_wolf();
   end
end

function NightlyWolfAttack:spawn_wolf() 
   local civ_home = self._civ_faction:get_home_location()

   local wolf = radiant.entities.create_entity('stonehearth:wolf')
   local leash_component = wolf:add_component('stonehearth:leash')
   
   if not civ_home then
      civ_home = Point3(0,0,0)
   end

   leash_component:set_location(civ_home)
   leash_component:set_radius(20)

   local radius = 30
   local spawn_location = Point3(civ_home.x + radius, 1, civ_home.z + radius)
   spawn_location.x = spawn_location.x + math.random(-3, 3)
   spawn_location.z = spawn_location.z + math.random(-3, 3)      

   radiant.terrain.place_entity(wolf, spawn_location)
end


return NightlyWolfAttack
