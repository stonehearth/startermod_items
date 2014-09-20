local Point3  = _radiant.csg.Point3

local DEFAULT_FACTION = 'civ'

local _all_entities = {}

local env = {}
function env.create_world(world_generator_script)
   env.session = {
      player_id = 'player_1',
      faction = 'civ',
      kingdom = 'stonehearth:kingdoms:ascendancy',
   }
   stonehearth.town:add_town(env.session)
   stonehearth.inventory:add_inventory(env.session)
   stonehearth.population:add_population(env.session)
   stonehearth.terrain:get_visible_region(env.session.faction) 
   stonehearth.terrain:get_explored_region(env.session.faction)

   local create_world = radiant.mods.load_script(world_generator_script)
   create_world(env)

   -- listen for every entity creation event so we can tear them all down between tests
   radiant.events.listen(radiant, 'radiant:entity:post_create', function(e)
         local entity = e.entity
         local id = entity:get_id()
         _all_entities[id] = entity
      end)

   radiant.events.listen(radiant, 'radiant:entity:post_destroy', function(e)
         _all_entities[e.entity_id] = nil
      end)

   env.town = stonehearth.town:get_town(env.session.player_id)

   env.create_enemy_kingdom()
end

function env.get_player_id()
   return 'player_1'
end

function env.get_player_session()
   return env.session
end

function env.create_enemy_kingdom()
   local session = {
      player_id = 'enemy',
      faction = 'raider',
      kingdom = 'stonehearth:kingdoms:goblin'
   }

   stonehearth.inventory:add_inventory(session)
   stonehearth.town:add_town(session)
   stonehearth.population:add_population(session)
end

function env.clear()
   for id, entity in pairs(_all_entities) do
      radiant.entities.destroy_entity(entity)      
   end
   _all_entities = {}
   
   -- move the camera to a decent spot
   local CAMERA_POSITION = Point3(11, 16, 39)
   local CAMERA_LOOK_AT = Point3(3.5, 1, 12.5)
   autotest_framework.ui.move_camera(CAMERA_POSITION, CAMERA_LOOK_AT)
end

function env.get_town()
   return env.town
end

local function apply_options_to_entity(entity, options)
   if options.faction then
      radiant.entities.set_faction(entity, options.faction)
   end
   if options.player_id then
      radiant.entities.set_player_id(entity, options.player_id)
   end
   if options.attributes then
      for name, value in pairs(options.attributes) do
         entity:get_component('stonehearth:attributes')
                  :set_attribute(name, value)
      end
   end
   if options.job then
      local job = options.job
      if not string.find(job, ':') and not string.find(job, '/') then
         -- as a convenience for autotest writers, stick the stonehearth:job on
         -- there if they didn't put it there to begin with
         job = 'stonehearth:jobs:' .. job
      end
      entity:add_component('stonehearth:job')
               :promote_to(job)
   end
   if options.weapon then
      env.equip_weapon(entity, options.weapon)
   end
end

function env.create_entity(x, z, uri, options)
   local entity = radiant.entities.create_entity(uri)
   apply_options_to_entity(entity, options or {})
   

   local place_options = {}
   if options and options.force_iconic ~= nil then
      place_options.force_iconic = options.force_iconic
   end

   local location = Point3(x, 1, z)
   radiant.terrain.place_entity(entity, location, place_options)
   return entity
end

function env.create_entity_cluster(x, y, w, h, uri, options)
   local entities = {}
   for i=x,x+w do
      for j=y,y+h do
         table.insert(entities, env.create_entity(i, j, uri, options))
      end
   end
   return entities
end

function env.create_person(x, z, options)
   options = options or {}
   local player_id = options.player_id or 'player_1'

   local pop = stonehearth.population:get_population(player_id)
   local person = pop:create_new_citizen()
   apply_options_to_entity(person, options)

   local location = Point3(x, 1, z)
   radiant.terrain.place_entity(person, location)
   return person
end

function env.create_stockpile(x, z, options)
   options = options or {}
   
   local faction = options.faction or DEFAULT_FACTION
   local size = options.size or { x = 2, y = 2}

   local location = Point3(x, 1, z)
   local inventory = stonehearth.inventory:get_inventory(env.session.player_id)

   return inventory:create_stockpile(location, size)
end

function env.create_trapping_grounds(x, z, options)
   options = options or {}
   
   local player_id = env.session.player_id
   local faction = options.faction or DEFAULT_FACTION
   local size = options.size or { x = 16, z = 16 }

   local location = Point3(x, 1, z)
   return stonehearth.trapping:designate_trapping_grounds(player_id, faction, location, size)
end

function env.equip_weapon(entity, weapon_uri)
   local weapon = radiant.entities.create_entity(weapon_uri)
   radiant.entities.equip_item(entity, weapon)
end

return env
