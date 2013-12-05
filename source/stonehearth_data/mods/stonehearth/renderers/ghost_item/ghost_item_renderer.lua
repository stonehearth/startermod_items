local Point3 = _radiant.csg.Point3

local GhostItemRenderer = class()

function GhostItemRenderer:__init(render_entity, data_store)
   self._ghost_item_rendered = nil

   self._data_store  = data_store
   self._promise = data_store:trace_data('rendering a ghost item')
                                 :on_changed(function()
                                    self:_update()
                                 end)
   self:_update()
end

function GhostItemRenderer:_update()
   local data = self._data_store:get_data()
   if data and data.full_sized_mod_url ~= '' then
      self._ghost_entity = radiant.entities.create_entity(data.full_sized_mod_url)
      self._ghost_entity:add_component('render_info')
         :set_material('materials/ghost_item.xml')
         :set_model_mode('blueprint')

      self._ghost_item_rendered = _radiant.client.create_render_entity(1, self._ghost_entity)

      local ghost_mob = self._ghost_entity:add_component('mob')
      ghost_mob:set_location_grid_aligned(Point3(data.location.x, data.location.y, data.location.z))
      ghost_mob:turn_to(data.rotation)
   end
end

function GhostItemRenderer:destroy()
   --Note: when we finally get an effect in here, destroying the main entity will nuke the effect too
   _radiant.client.destroy_authoring_entity(self._ghost_entity:get_id())
end

return GhostItemRenderer