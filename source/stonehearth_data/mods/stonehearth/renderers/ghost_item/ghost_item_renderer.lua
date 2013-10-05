local Point3 = _radiant.csg.Point3

local GhostItemRenderer = class()

function GhostItemRenderer:__init(render_entity, data_store)
   self._ghost_item_rendered = nil

   self._data_store  = data_store
   self._promise = data_store:trace('rendering a ghost item')
   self._promise:on_changed(function()
         self:_update()
      end)
   self:_update()
end

function GhostItemRenderer:_update()
   local data = self._data_store:get_data()
   if data and data.full_sized_mod_url ~= '' then
      self._ghost_entity = radiant.entities.create_entity(data.full_sized_mod_url)

      --TODO: debug this, not actually appearing in the world at the moment
      local ghost_entity_unit_info = self._ghost_entity:add_component('unit_info')
      ghost_entity_unit_info:set_display_name(data.unit_info_name)
      ghost_entity_unit_info:set_description(data.unit_info_description)
      ghost_entity_unit_info:set_icon(data.unit_info_icon)

      --self._particle_effect = radiant.effects.run_effect(self._ghost_entity, '/stonehearth/data/effects/firepit_effect')

      self._ghost_item_rendered = _radiant.client.create_render_entity(1, self._ghost_entity)

      local ghost_mob = self._ghost_entity:add_component('mob')
      ghost_mob:set_location_grid_aligned(Point3(data.location.x, data.location.y, data.location.z))
      ghost_mob:turn_to(data.rotation)
      --radiant.terrain.place_entity(self._ghost_entity, data.location)
   end
end

function GhostItemRenderer:destroy()
   --Hopefully destroying the entity will remove the render entity
   --self._particle_effect:stop()
   _radiant.client.destroy_authoring_entity(self._ghost_entity:get_id())
end

return GhostItemRenderer