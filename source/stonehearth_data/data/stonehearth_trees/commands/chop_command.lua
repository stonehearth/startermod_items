
function chop(player, args)
   local tree = radiant.entities.get_entity(args.tree)
   radiant.log.info('chopping down tree %s', tostring(tree))
   local inventory = radiant.mods.require("/stonehearth_inventory/").get_inventory(player.faction)
   if inventory then
      inventory:harvest_tree(tree)
   end
end

return chop
