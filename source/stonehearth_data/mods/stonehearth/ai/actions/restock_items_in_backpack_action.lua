local Entity = _radiant.om.Entity

local RestockItemsInBackpack = class()
RestockItemsInBackpack.name = 'restock items in backpack'
RestockItemsInBackpack.does = 'stonehearth:restock_items_in_backpack'
RestockItemsInBackpack.args = {}
RestockItemsInBackpack.version = 2
RestockItemsInBackpack.priority = 1

local ai = stonehearth.ai
return ai:create_compound_action(RestockItemsInBackpack)
   :execute('stonehearth:find_stockpile_for_backpack_item')
   :execute('stonehearth:follow_path', {
      path = ai.PREV.path
   })
   :execute('stonehearth:reserve_entity_destination', {
      entity = ai.BACK(2).stockpile,
      location = ai.BACK(2).path:get_destination_point_of_interest()
   })
   :execute('stonehearth:pickup_item_type_from_backpack', {
      filter_fn = ai.BACK(3).stockpile:add_component('stonehearth:stockpile'):get_filter(),
      descripton = 'unloading backpack'
   })
   :execute('stonehearth:drop_carrying_adjacent', { location = ai.BACK(2).location })
   -- Immediately after dropping the item into the stockpile, notify it that we've added something
   -- so it can update it's "valid spaces to drop stuff" region immediately.  Waiting for a 
   -- lua trace on the terrain to fire would allow additional code to run after we've unreserved the
   -- spot, but before we've marked it as occupied!
   :execute('stonehearth:call_method', {
      obj = ai.BACK(5).stockpile:add_component('stonehearth:stockpile'),
      method = 'notify_restock_finished',
      args = { ai.BACK(3).location }
   })
