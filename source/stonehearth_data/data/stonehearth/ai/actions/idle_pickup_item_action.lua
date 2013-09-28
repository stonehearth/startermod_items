local IdlePickupItemAction = class()

IdlePickupItemAction.name = 'stonehearth.actions.idle_pickup_item_action'
IdlePickupItemAction.does = 'stonehearth.activities.idle'
IdlePickupItemAction.priority = 10

function IdlePickupItemAction:__init(ai, entity, item)
   self._item = item
end

function IdlePickupItemAction:run(ai, entity)
   ai:execute('stonehearth.activities.pickup_item', self._item)
end

return IdlePickupItemAction