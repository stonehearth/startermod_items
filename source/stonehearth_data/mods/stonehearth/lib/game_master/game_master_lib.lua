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

local function create_or_load_citizens(population, info, origin)
   local citizens = {}

   if info.from_population then
      local min = info.from_population.min or 1
      local max = info.from_population.max or 1
      local num = rng:get_int(min, max)
      
      for i = 1, num do
         local citizen = population:create_new_citizen(info.from_population.role)
         citizens[citizen:get_id()] = citizen
      end
   elseif info.from_ctx then
      radiant.not_yet_implemented()
   end

   return citizens
end

function game_master_lib.create_citizens(population, info, origin)
   local citizens = create_or_load_citizens(population, info, origin)
  
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

         -- trigger at the end when the citizen is fully initialized
         radiant.events.trigger_async(citizen, 'stonehearth:game_master:citizen_added', {})
      end
   end

   return citizens
end

return game_master_lib