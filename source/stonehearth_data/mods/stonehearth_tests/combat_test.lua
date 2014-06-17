local MicroWorld = require 'lib.micro_world'
local Point3 = _radiant.csg.Point3
local rng = _radiant.csg.get_default_rng()

local CombatTest = class(MicroWorld)

function CombatTest:__init()
   self[MicroWorld]:__init()
   self:create_world()
   self:create_enemy_kingdom()

   -- for i = -10, 10 do
   --    self:place_item('stonehearth:small_boulder', i, 0)
   --    if math.abs(i) <= 5 and i ~= 0 then
   --       self:place_item('stonehearth:small_boulder', 0, i)
   --    end
   -- end

   local citizens = {
      self:place_citizen(-15, -15, 'stonehearth:professions:footman', 'stonehearth:wooden_sword'),
      self:place_citizen( -7, -15, 'stonehearth:professions:footman', 'stonehearth:wooden_sword'),
      self:place_citizen(  1, -15, 'stonehearth:professions:footman', 'stonehearth:wooden_sword'),
      self:place_citizen(  9, -15, 'stonehearth:professions:footman', 'stonehearth:wooden_sword'),
   }

   local enemies = {
      self:place_enemy( -9, 15, 'stonehearth:wooden_sword'),
      self:place_enemy( -1, 15, 'stonehearth:wooden_sword'),
      self:place_enemy(  7, 15, 'stonehearth:wooden_sword'),
      self:place_enemy( 15, 15, 'stonehearth:wooden_sword'),
   }

   -- self:at(3000,
   --    function ()
   --       self:kill(citizens[1])
   --    end
   -- )

   --Test goblins attacking others (death notifications, etc)
   --[[
   --add a banner
   local player_id = citizens[1]:get_component('unit_info'):get_player_id()
   local town = stonehearth.town:get_town(player_id)
   local location = Point3(7, 0, 7)
   local banner_entity = radiant.entities.create_entity('stonehearth:camp_standard')
   radiant.terrain.place_entity(banner_entity, location)
   town:set_banner(banner_entity)

   -- Introduce a new person
   self:at(20000,  function()
         stonehearth.dynamic_scenario:force_spawn_scenario('Immigration')
      end)
   ]]

end

function CombatTest:create_enemy_kingdom()
   local session = {
      player_id = 'game_master',
      faction = 'raider',
      kingdom = 'stonehearth:kingdoms:golden_conquering_arm'
   }

   stonehearth.inventory:add_inventory(session)
   self._enemy_population = stonehearth.population:add_population(session)
end

function CombatTest:place_enemy(x, z, weapon)
   local enemy = self._enemy_population:create_new_citizen()
   self:equip_weapon(enemy, weapon)
   self:inject_enemy_observer(enemy)
   radiant.terrain.place_entity(enemy, Point3(x, 1, z))
   return enemy
end

function CombatTest:equip_weapon(entity, weapon_uri)
   local weapon = radiant.entities.create_entity(weapon_uri)
   local equipment = entity:add_component('stonehearth:equipment')
   equipment:equip_item(weapon, 'melee_weapon')

   -- HACK: remove the talisman glow effect from the weapon
   -- might want to remove other talisman related commands as well
   -- TODO: make the effects and commands specific to the model variant
   weapon:remove_component('effect_list')
end

function CombatTest:inject_enemy_observer(entity)
   stonehearth.ai:inject_ai(entity, { observers = { "stonehearth:observers:enemy_observer" }})
end

function CombatTest:kill(entity)
   local attributes_component = entity:add_component('stonehearth:attributes')
   attributes_component:set_attribute('health', 0)
end

return CombatTest
