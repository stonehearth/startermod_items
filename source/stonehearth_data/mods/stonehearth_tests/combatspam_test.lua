local MicroWorld = require 'lib.micro_world'
local Point3 = _radiant.csg.Point3
local rng = _radiant.csg.get_default_rng()

local CombatSpamTest = class(MicroWorld)

function CombatSpamTest:__init()
   self[MicroWorld]:__init(64)
   self:create_world()
   self:create_enemy_kingdom()
   self._player_population = stonehearth.population:get_population('player_1')

   self._footmen_origin = Point3(-30, 0, -30)
   self._goblin_origin = Point3(30, 0, 30)

   self:at(1000, function()
         self:place_footmen(self._footmen_origin, 9)
         self:place_goblins(self._goblin_origin, 25)
      end)

   radiant.events.listen(radiant, 'radiant:entity:post_destroy', function()
      end)
end

function CombatSpamTest:place_footmen(location, number)
   local entities = {}
   for i = 1, number do
      local entity = self._player_population:create_new_citizen()
      entity:add_component('stonehearth:job'):promote_to('stonehearth:jobs:footman')
      table.insert(entities, entity)

      radiant.events.listen_once(entity, 'radiant:entity:pre_destroy', function()
            self:on_footman_died()
         end)
   end

   self:place_entities(entities, location)
end

function CombatSpamTest:place_goblins(location, number)
   local entities = {}
   for i = 1, number do
      local entity = self._enemy_population:create_new_citizen()
      self:equip_weapon(entity, 'stonehearth:weapons:jagged_cleaver')
      table.insert(entities, entity)

      radiant.events.listen_once(entity, 'radiant:entity:pre_destroy', function()
            self:on_goblin_died()
         end)
   end

   self:place_entities(entities, location)
end

function CombatSpamTest:place_entities(entities, location, separation)
   separation = separation or 2
   local num_entities = #entities
   local length = math.ceil(math.sqrt(num_entities))
   local center = math.floor((1+length)/2)
   local offset = Point3.zero
   local count = 1

   for z = 1, length do
      for x = 1, length do
         offset.x = (x-center)*separation
         offset.z = (z-center)*separation

         radiant.terrain.place_entity(entities[count], location + offset)

         if count == num_entities then
            return
         end
         count = count + 1
      end
   end
end

function CombatSpamTest:on_footman_died()
   self:place_footmen(self._footmen_origin, 1)
end

function CombatSpamTest:on_goblin_died()
   self:place_goblins(self._goblin_origin, 1)
end

function CombatSpamTest:create_enemy_kingdom()
   local session = {
      player_id = 'game_master',
      faction = 'raider',
   }

   stonehearth.inventory:add_inventory(session)
   stonehearth.town:add_town(session)
   self._enemy_population = stonehearth.population:add_population(session, 'stonehearth:kingdoms:goblin')
end

function CombatSpamTest:equip_weapon(entity, weapon_uri)
   local weapon = radiant.entities.create_entity(weapon_uri)
   radiant.entities.equip_item(entity, weapon)
end

function CombatSpamTest:kill(entity)
   local attributes_component = entity:add_component('stonehearth:attributes')
   attributes_component:set_attribute('health', 0)
end

return CombatSpamTest
