local Color4 = _radiant.csg.Color4

local OreBlockRenderer = class()

function OreBlockRenderer:__init(render_entity)
   self._render_entity = render_entity
   self._entity = self._render_entity:get_entity()
   self._parent_node = self._render_entity:get_node()
   self._edge_color = Color4(255, 255, 0, 255)
   self._face_color = Color4(0, 0, 0, 1)
end

function OreBlockRenderer:destroy()
   self:_destroy_outline_node()
end

function OreBlockRenderer:set_region(region)
   self._region = region
   self:_update()
end

function OreBlockRenderer:_destroy_outline_node()
   if self._outline_node then
      self._outline_node:destroy()
      self._outline_node = nil
   end
end

function OreBlockRenderer:_update()
   self:_destroy_outline_node()

   self._outline_node = _radiant.client.create_region_outline_node(self._parent_node, self._region, self._edge_color, self._face_color, 'materials/transparent.material.json', true)
end

return OreBlockRenderer
