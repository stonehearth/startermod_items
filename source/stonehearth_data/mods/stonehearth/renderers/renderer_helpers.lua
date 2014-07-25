local RendererHelpers = class()

function RendererHelpers.set_ghost_mode(render_entity, ghost_mode)
   local material_kind

   if ghost_mode then
      material_kind = 'hud'
      render_entity:add_query_flag(_radiant.renderer.QueryFlags.UNSELECTABLE)
   else
      material_kind = 'default'
      render_entity:remove_query_flag(_radiant.renderer.QueryFlags.UNSELECTABLE)
   end

   local material = render_entity:get_material_path(material_kind)           
   render_entity:set_material_override(material)
end

return RendererHelpers
