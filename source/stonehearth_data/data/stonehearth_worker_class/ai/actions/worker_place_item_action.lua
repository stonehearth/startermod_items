local Point3 = _radiant.csg.Point3
local PlaceItemAction = class()

PlaceItemAction.name = 'stonehearth.actions.place_item'
PlaceItemAction.does = 'stonehearth.activities.place_item'
PlaceItemAction.priority = 10
--TODO we need a scale to  describe relative importance;
--Putting things in stockpiles is at 5, so this has to be greater

--- Put the proxy object we're carrying down at a location
-- Also, reinflate it (assuming it's a proxy object)
-- @param location The place to drop the object
-- @param rotation The degree at which the object should appear

function PlaceItemAction:run(ai, entity, path, rotation)
   local proxy_entity = radiant.entities.get_carrying(entity)
   assert(proxy_entity, "This worker is trying to drop something he isn't carrying!")

   local drop_location = path:get_finish_point()
   ai:execute('stonehearth.activities.follow_path', path)
   ai:execute('stonehearth.activities.drop_carrying', drop_location)
   radiant.entities.turn_to(proxy_entity, rotation)

   --TODO: replace this with hammer
   ai:execute('stonehearth.activities.run_effect', 'work')

   -- assuming it's a proxy entity
   local proxy_component = proxy_entity:get_component('stonehearth_items:placeable_item_proxy')
   if proxy_component then
      --Get the full sized entity
      local full_sized_entity = proxy_component:get_full_sized_entity()
      local faction = entity:get_component('unit_info'):get_faction()
      full_sized_entity:get_component('unit_info'):set_faction(faction)

      -- Remove the icon (TODO: what about stacked icons? Remove just one of the stack?)
      radiant.terrain.remove_entity(proxy_entity)

      -- Place the item in the world
      radiant.terrain.place_entity(full_sized_entity, drop_location)
      radiant.entities.turn_to(full_sized_entity, rotation)
   end
end

return PlaceItemAction