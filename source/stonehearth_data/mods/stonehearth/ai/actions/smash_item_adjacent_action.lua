local Entity = _radiant.om.Entity
local SmashItemAdjacent = class()

SmashItemAdjacent.name = 'smash an item'
SmashItemAdjacent.does = 'stonehearth:smash_item_adjacent'
SmashItemAdjacent.args = {
   item = Entity
}
SmashItemAdjacent.version = 2
SmashItemAdjacent.priority = 1

local log = radiant.log.create_logger('actions.smash_item')

function SmashItemAdjacent:run(ai, entity, args)
   local item = args.item   
   radiant.check.is_entity(item)

   if not radiant.entities.is_adjacent_to(entity, item) then
      ai:abort('%s is not adjacent to %s', tostring(entity), tostring(item))
      return
   end

   log:info("%s picking up %s", entity, item)
   radiant.entities.turn_to_face(entity, item)
   ai:execute('stonehearth:run_effect', { effect = 'work', times = 3 })
   ai:unprotect_argument(item)
   radiant.entities.kill_entity(item)
end

return SmashItemAdjacent
