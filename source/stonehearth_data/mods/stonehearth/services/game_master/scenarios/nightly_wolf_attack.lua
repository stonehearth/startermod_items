local NightlyWolfAttack = class()

local calendar = stonehearth.calendar
local Point3 = _radiant.csg.Point3

function NightlyWolfAttack:__init()
   self._wolves = {}
   radiant.events.listen(calendar, 'stonehearth:hourly', self, self.on_hourly)
end

function NightlyWolfAttack:load_faction()
   self._civ_faction = stonehearth.population:get_faction('stonehearth:factions:ascendancy')
end

function NightlyWolfAttack:on_hourly()
   if date.hour == 2 then
      if not self._civ_faction then
         self:load_faction()
      end

      for i = 1, 1 do
         self:spawn_wolf();
      end
   elseif date.hour == 6 then
      self:despawn_wolves();
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

   if not self._spawn_location then
      self._spawn_location = Point3(civ_home.x + radius, 1, civ_home.z + radius)
   end

   --spawn_location.x = spawn_location.x + math.random(-3, 3)
   --spawn_location.z = spawn_location.z + math.random(-3, 3)      

   radiant.terrain.place_entity(wolf, self._spawn_location)

   table.insert(self._wolves, wolf)
end

function NightlyWolfAttack:despawn_wolves() 
   for i, wolf in ipairs(self._wolves) do
      -- tell the wolf to return home
      local leash_component = wolf:add_component('stonehearth:leash')
      leash_component:set_location(self._spawn_location)
      radiant.entities.destroy_entity(wolf)
      table.remove(self._wolves, i)
   end
end


return NightlyWolfAttack
