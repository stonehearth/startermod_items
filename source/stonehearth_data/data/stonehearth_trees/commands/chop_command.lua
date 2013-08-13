
function chop(player, args)
   local tree = radiant.entities.get_entity(args.tree)
   radiant.log.info('chopping down tree %s', tostring(tree))
   local stonehearth_trees = radiant.mods.require("/stonehearth_trees/")
   stonehearth_trees.harvest_tree(player.faction, tree)
end

return chop
