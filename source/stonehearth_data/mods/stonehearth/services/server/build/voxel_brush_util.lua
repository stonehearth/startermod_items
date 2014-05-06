local voxel_brush_util = _radiant.csg.Point3
local Point3 = _radiant.csg.Point3
local Point3f = _radiant.csg.Point3f
local Region2 = _radiant.csg.Region2

-- paint_mode is optional.  if not specified, we'll use the paint mode
-- inside the construction_data
function voxel_brush_util.create_brush(construction_data, paint_mode)
   assert(construction_data)

   local brush   
   if construction_data.use_nine_grid then
      assert(construction_data.nine_grid_region)

      -- xxx: i wish we didnt have to do all this fixup nonsense.  fixing
      -- it requires being able to convert json back to value types, which
      -- is somewhat tricky...
      if (type(construction_data.nine_grid_region) == 'table') then
         local t = construction_data.nine_grid_region
         construction_data.nine_grid_region = Region2()
         construction_data.nine_grid_region:load(t)
      end
      
      brush = _radiant.voxel.create_nine_grid_brush(construction_data.brush)
      brush:set_grid_shape(construction_data.nine_grid_region)
      --[[
      if construction_data.grid9_tile_mode then
         brush:set_grid9_tile_mode(construction_data.grid9_tile_mode)
      end
      if construction_data.grid9_slope_mode then
         brush:set_grid9_tile_mode(construction_data.grid9_slope_mode)
      end
      ]]
   else 
      brush = _radiant.voxel.create_brush(construction_data.brush)
   end
   
   if construction_data.normal then
      local normal = Point3(construction_data.normal.x,
                            construction_data.normal.y,
                            construction_data.normal.z)
      brush:set_normal(normal)
   end

   if not paint_mode then
      paint_mode = construction_data.paint_mode
   end
   if paint_mode == 'blueprint' then
      brush:set_paint_mode(_radiant.voxel.QubicleBrush.Opaque)
   end
   brush:set_clip_whitespace(true)

   return brush
end

-- paint_mode is optional.  if not specified, we'll use the paint mode
-- inside the construction_data
function voxel_brush_util.create_construction_data_node(parent_node, entity, region, construction_data, paint_mode)
   local unique_renderable
   if region and construction_data.brush then
      local stencil = region:get()
      if stencil then
         local render_info = entity:get_component('render_info')
         local material = render_info and render_info:get_material() or 'materials/voxel.material.xml'

         paint_mode = paint_mode and paint_mode or construction_data.paint_mode
         local brush = voxel_brush_util.create_brush(construction_data, paint_mode) 
         local model = brush:paint_through_stencil(stencil)

         unique_renderable = _radiant.client.create_voxel_node(parent_node, model, material, Point3f(0.5, 0, 0.5))
      end
   end
   return unique_renderable
end

return voxel_brush_util
