local api = {}

function api.chop(tree, player_faction)
   radiant.log.info('chopping down tree %s', tostring(tree))
   local inventory = radiant.mods.require("mod://stonehearth_inventory/").get_inventory(player_faction)
   if inventory then
      inventory:harvest_tree(tree)
   end
end

return api
