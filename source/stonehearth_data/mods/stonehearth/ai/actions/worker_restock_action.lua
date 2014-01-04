local WorkerRestockAction = class()

WorkerRestockAction.name = 'restock'
WorkerRestockAction.does = 'stonehearth:restock'
WorkerRestockAction.version = 1
WorkerRestockAction.priority = 5

function WorkerRestockAction:run(ai, entity, path, stockpile)
   --When restocking is interrupted, sometimes the interrupting
   --action drops the object we're carrying, rendering us unfit to 
   --continue with this action when we pick it back up again.
   --Check for this. 
   if not radiant.entities.is_carrying(entity) then
      ai:abort()
   end

   local drop_location = path:get_destination_point_of_interest()
   ai:execute('stonehearth:follow_path', path)
   ai:execute('stonehearth:drop_carrying', drop_location)
   self._reserved = nil
end

return WorkerRestockAction
