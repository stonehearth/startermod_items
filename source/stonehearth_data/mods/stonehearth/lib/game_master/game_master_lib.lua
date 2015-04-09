local i18n = require 'lib.i18n.i18n'()
local rng = _radiant.csg.get_default_rng()

local Point3 = _radiant.csg.Point3

local game_master_lib = {}

local function _compile_bulletin_text(text, ctx)
   if type(text) == 'table' then
      assert(text[1])
      text = text[rng:get_int(1, #text)]
   end
   return i18n:format_string(text, ctx)
end


-- will be of the form...
--   nodes = {
--      key1 = {
--         bulletin = {
--         }
--      }   
--}
function game_master_lib.compile_bulletin_nodes(nodes, ctx)
   for _, node in pairs(nodes) do
      local bulletin = node.bulletin
      if bulletin then
         bulletin.title          = _compile_bulletin_text(bulletin.title, ctx)
         bulletin.message        = _compile_bulletin_text(bulletin.message, ctx)
         bulletin.portrait       = _compile_bulletin_text(bulletin.portrait, ctx)
         bulletin.dialog_title   = _compile_bulletin_text(bulletin.dialog_title, ctx)
      end
   end
end

function game_master_lib.create_context(node_name, node, parent_node)
   assert(type(node_name) == 'string')
   assert(radiant.util.is_instance(node))
   
   local parent_ctx
   if parent_node then
      parent_ctx = parent_node._sv.ctx
      assert(parent_ctx)

      table.insert(parent_node._sv.child_nodes, node)
      parent_node.__saved_variables:mark_changed()
   end

   local ctx = radiant.create_controller('stonehearth:game_master:node_ctx', node, parent_ctx)
   -- stash some stuff in the node...
   node._sv.ctx = ctx
   node._sv.node_name = node_name
   node._sv.child_nodes = { n = 0 }
   node.__saved_variables:mark_changed()

   return ctx
end

--Creates an entity and adds optional info from the json file
--Currently supports unit_info (name/description)
function game_master_lib.create_entity(info, player_id)
   local entity = radiant.entities.create_entity(info.uri, {owner = player_id })
   if info.unit_info and info.unit_info.name then
      radiant.entities.set_name(entity, info.unit_info.name)
   end
   if info.unit_info and info.unit_info.description then
      radiant.entities.set_description(entity, info.unit_info.description)
   end
   return entity
end

--If the citizen should be created from scratch, from the population, do that
--Otherwise, the citizen should exist already in the ctx, and should be reused from there
local function _create_or_load_citizens(population, info, origin, ctx)
   local citizens = {}

   if info.from_population then
      local min = info.from_population.min or 1
      local max = info.from_population.max or 1
      local num = rng:get_int(min, max)
      
      for i = 1, num do
         local citizen = population:create_new_citizen(info.from_population.role)
         citizens[citizen:get_id()] = citizen
      end
   elseif info.from_ctx and ctx then
      --Retrieve the citizen from the context. Can only create 1 citizen from context per block
      local entity = ctx:get(info.from_ctx)
      if entity and entity:is_valid() then
         citizens[entity:get_id()] = entity
      end
   end
   return citizens
end

-- Given data about citzens, spit out a citizen
-- In info: 
--    from_population means that we create via the population service.
--       Can also pass in role, location, and min/max if we want to create more than one entity
--    from_ctx means that we re-use something in the ctx. Pass a string path from ctx to the entity
--    equipment: allows you to add equipment to the entity
--    attributes: allows you to reset specific attributes for this entity
-- @param population: the population that these citizens should belong to
-- @param info: details about the people we're going to create
-- @param ctx : optional, the ctx if we have it
-- @returns the array of citizens it managed to create
function game_master_lib.create_citizens(population, info, origin, ctx)
   local citizens = _create_or_load_citizens(population, info, origin, ctx)
  
   for id, citizen in pairs(citizens) do
      -- info.job: promote the citizen to the proper job
      if info.job then
         citizen:add_component('stonehearth:job')
                     :promote_to(info.job)
      end

      -- info.equipment: gear up!
      if info.equipment then
         local ec = citizen:add_component('stonehearth:equipment')
         for _, piece in pairs(info.equipment) do
            -- piece is either the uri to the piece or an array of candidates
            if type(piece) == 'table' then
               piece = piece[rng:get_int(1, #piece)]            
            end
            ec:equip_item(piece)
         end
      end

      -- info.loot_drops: what does the guy drop when it dies?
      if info.loot_drops then
         citizen:add_component('stonehearth:loot_drops')
                     :set_loot_table(info.loot_drops)
      end

      -- info.combat_leash_range: set the leash on the citizen if provide.  the
      -- leash prevents the mob from being kited too far off their home location
      if info.combat_leash_range then
         citizen:add_component('stonehearth:combat_state')
                     :set_attack_leash(origin, info.combat_leash_range)
      end


      --if info.attributes, then add these attributes to the entity
      --this should allow us to tweak the attributes of specific entities
      --we are only allowed to add basic attributes
      if info.attributes then
         local attrib_component = citizen:add_component('stonehearth:attributes')
         for name, value in pairs(info.attributes) do
            attrib_component:set_attribute(name, value)
         end
      end

      -- if we created the citizen, place him on the terrain now (after he's been fully initialized)
      if info.from_population then
         local location = origin
         if info.from_population.location then
            local x = info.from_population.location.x or 0
            local y = info.from_population.location.y or 0
            local z = info.from_population.location.z or 0

            location = location + Point3(x, y, z)

            -- spawn within some range of the location
            if info.from_population.range then
               location = radiant.terrain.find_placement_point(location, 1, info.from_population.range)
            end
            
         end
         -- put the new citizen in the world and return it
         radiant.terrain.place_entity(citizen, location)

      end

      -- trigger at the end when the citizen is fully initialized
      radiant.events.trigger_async(citizen, 'stonehearth:game_master:citizen_config_complete', {})
   end

   return citizens
end

return game_master_lib