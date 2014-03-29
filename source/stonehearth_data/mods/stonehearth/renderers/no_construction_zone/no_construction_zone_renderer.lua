local Point3 = _radiant.csg.Point3
local Color4 = _radiant.csg.Color4

local NoConstructionZoneRenderer = class()

function NoConstructionZoneRenderer:update(render_entity, ncz)
   self:_destroy_node()
   
   if ncz.region2 then
      local parent = render_entity:get_node()
      local cursor = ncz.region2:get()
      self._node = _radiant.client.create_designation_node(parent, cursor, Color4(32, 32, 32, 255), Color4(128, 128, 128, 255));
   end
end

function NoConstructionZoneRenderer:destroy()
   self:_destroy_node()
end

function NoConstructionZoneRenderer:_destroy_node()
   if self._node then
      h3dRemoveNode(self._node)
      self._node = nil
   end
end

return NoConstructionZoneRenderer