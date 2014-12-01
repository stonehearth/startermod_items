local voxel_brush_util = {}
local Point3 = _radiant.csg.Point3
local Region3 = _radiant.csg.Region3
local Region2 = _radiant.csg.Region2
local NineGridBrush = _radiant.voxel.NineGridBrush


local MODEL_OFFSET = Point3(0, 0, 0)

function voxel_brush_util.brush_from_uri(uri)
   local e = radiant.entities.create_entity(uri)
   local c = e:get_component('stonehearth:construction_data')
   assert(c)
   local brush = c:get_brush()
   assert(brush)
   radiant.entities.destroy_entity(e)

   return brush
end

function voxel_brush_util.create_brush(construction_data)
   assert(construction_data)

   local brush   
   if construction_data.use_nine_grid then
      assert(construction_data.nine_grid_region)

      -- xxx: i wish we didnt have to do all this fixup nonsense.  fixing
      -- it requires being able to convert json back to value types, which
      -- is somewhat tricky...
      if type(construction_data.nine_grid_region) == 'table' then
         local t = construction_data.nine_grid_region
         construction_data.nine_grid_region = Region2()
         construction_data.nine_grid_region:load(t)
      end
      
      brush = _radiant.voxel.create_nine_grid_brush(construction_data.brush)
                                 :set_grid_shape(construction_data.nine_grid_region)
                                 :set_slope(construction_data.nine_grid_slope or 1)
      if construction_data.nine_grid_slope then
         brush:set_slope(construction_data.nine_grid_slope)
      end
      if construction_data.nine_grid_max_height then
         brush:set_max_height(construction_data.nine_grid_max_height)
      end
      if construction_data.nine_grid_y_offset then
         brush:set_y_offset(construction_data.nine_grid_y_offset)
      end
      if construction_data.nine_grid_gradiant then
         local flags = 0
         for _, f in ipairs(construction_data.nine_grid_gradiant) do
            if f == "front" then
               flags = flags + NineGridBrush.Front
            elseif f == "back" then
               flags = flags + NineGridBrush.Back
            elseif f == "left" then
               flags = flags + NineGridBrush.Left
            elseif f == "right" then
               flags = flags + NineGridBrush.Right
            end
         end
         brush:set_gradiant_flags(flags)
      end
      
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
      if construction_data.normal then
         local normal = Point3(construction_data.normal.x,
                               construction_data.normal.y,
                               construction_data.normal.z)
         brush:set_normal(normal)
      end
   end  

   brush:set_clip_whitespace(true)

   return brush
end

function voxel_brush_util.create_construction_data_node(parent_node, entity, region, construction_data)
   local render_node
   if region then
      local model
      local stencil = region:get()
      if stencil then
         local render_info = entity:get_component('render_info')
         local material = render_info and render_info:get_material() or 'materials/voxel.material.xml'

         if construction_data:get_paint_through_blueprint() then
            local blueprint = construction_data:get_blueprint_entity()
            model = blueprint:get_component('destination'):get_region():get():intersect_region(stencil)
         else
            local brush = construction_data:create_voxel_brush()
            if brush then
               model = brush:paint_through_stencil(stencil)
            else
               model = stencil
            end
         end
         render_node = _radiant.client.create_voxel_node(parent_node, model, material, MODEL_OFFSET)
      end
   end
   return render_node
end

return voxel_brush_util
