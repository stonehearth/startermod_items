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

function game_master_lib.create_context(node_name, node, parent_ctx)
   assert(type(node_name) == 'string')
   assert(radiant.util.is_instance(node))
   
   local ctx = radiant.create_controller('stonehearth:game_master:node_ctx', parent_ctx)
   ctx.node = node
   ctx.node_name = node_name
   return ctx
end

function game_master_lib.create_citizen(population, info, origin)
   local citizen = population:create_new_citizen()

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

   -- info.location: compute the offset from the origin to place the citizen
   if info.location then
      local x = info.location.x or 0
      local y = info.location.y or 0
      local z = info.location.z or 0
      origin = origin + Point3(x, y, z)
   end

   -- info.combat_leash_range: set the leash on the citizen if provide.  the
   -- leash prevents the mob from being kited too far off their home location
   if info.combat_leash_range then
      citizen:add_component('stonehearth:combat_state')
                  :set_attack_leash(origin, info.combat_leash_range)
   end

   -- finally, put the new citizen in the world and return it
   radiant.terrain.place_entity(citizen, origin)

   return citizen
end

return game_master_lib