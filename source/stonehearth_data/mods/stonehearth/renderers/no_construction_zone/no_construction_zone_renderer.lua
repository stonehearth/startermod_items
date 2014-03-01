local Point3 = _radiant.csg.Point3
local Color3 = _radiant.csg.Color3

local NoConstructionZoneRenderer = class()

function NoConstructionZoneRenderer:update(render_entity, data_store)
   self._parent_node = render_entity:get_node()
   self._data_store = data_store

   self._promise = data_store:trace_data('rendering')
                                 :on_changed(function()
                                    self:_update()
                                 end)
   self:_update()
end

function NoConstructionZoneRenderer:_clear()
   if self._designation_node then
      h3dRemoveNode(self._designation_node)
      self._designation_node = nil
   end
end

function NoConstructionZoneRenderer:_update()
   self:_clear()
   local data = self._data_store:get_data()
   if data.region2 then
      local cursor = data.region2:get()
      self._designation_node = _radiant.client.create_designation_node(self._parent_node, cursor, Color3(32, 32, 32), Color3(128, 128, 128));
   end
end

function NoConstructionZoneRenderer:destroy()
   self:_clear()
end


return NoConstructionZoneRenderer