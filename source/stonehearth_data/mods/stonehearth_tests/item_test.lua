local MicroWorld = require 'lib.micro_world'
local Point3 = _radiant.csg.Point3

local PlacementTest = class(MicroWorld)

function PlacementTest:__init()
   self[MicroWorld]:__init(64)
   self:create_world()

   local citizen = self:place_citizen(12, 12)
   
   local player_id = citizen:get_component('unit_info'):get_player_id()
   
   self:_place_all_items()
   self:place_item_cluster('stonehearth:resources:wood:oak_log', 8, 8, 7, 7)
   
   self:at(10,  function()
         self:place_stockpile_cmd(player_id, -32, -32, 64, 64)
      end)

end

function PlacementTest:_place_all_items()
   local mods = radiant.resources.get_mod_list()
   local x = -30
   local z = -30
   
   -- for each mod
   for i, mod in ipairs(mods) do
      local manifest = radiant.resources.load_manifest(mod)
      -- for each alias
      if manifest.aliases then
         for alias, uri in pairs(manifest.aliases) do
            -- is it placeable?
            local full_alias = mod .. ':' .. alias
            local path = radiant.resources.convert_to_canonical_path(full_alias)

            if path ~= nil and self:_alias_is_item(path) then
               self:place_item(full_alias, x, z)
               x = (x + 1) % 32
               if x == 0 then
                  z = z + 1
               end
            end
         end
      end
   end
end

function PlacementTest:_alias_is_item(path)
   if string.sub(path, -5) ~= '.json' then
      return false
   end

   local json = radiant.resources.load_json(path)

   if not json.type or json.type ~= 'entity' then
      return false
   end

   if json['components'] ~= nil and json['components']['item'] ~= nil then
      return true
   end

   return false
end


return PlacementTest

