local ColumnBlueprintRenderer = class()

function ColumnBlueprintRenderer:__init(render_entity, json)
   self._region_node = h3dRadiantCreateRegionNode(render_entity:get_node(), 'column blueprint node')   
   local rc = render_entity:get_entity():get_component('region_collision_shape')
   if rc then
      self._promise = rc:trace_region('updating column render shape')
                        :on_changed(function (r)
                              self:_update_region(r)
                           end)           
      self:_update_region(rc:get_region():get()) -- xxx: NO NO!  the trace should fire once automagically
   end
end

function ColumnBlueprintRenderer:_update_region(r)
   h3dRadiantUpdateRegionNode(self._region_node, r)
end

function ColumnBlueprintRenderer:__destroy()
end

return ColumnBlueprintRenderer
