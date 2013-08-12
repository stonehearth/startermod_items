
function promote(player, args)
   local target_talisman = radiant.entities.get_entity(args.talisman)
   --TODO: produce the target person
   radiant.events.broadcast_msg('stonehearth_item.promote_citizen', {talisman = target_talisman, target_person = 14})
end

return promote
