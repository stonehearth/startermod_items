local voxel_brush_util = {}
local Point3 = _radiant.csg.Point3
local Region3 = _radiant.csg.Region3
local Region2 = _radiant.csg.Region2
local NineGridBrush = _radiant.voxel.NineGridBrush

local MODEL_OFFSET = Point3(0, 0, 0)

function voxel_brush_util.create_construction_data_node(parent_node, entity, region, construction_data)
   local render_node
   if region then
      local model
      local stencil = region:get()
      if stencil then
         local render_info = entity:get_component('render_info')
         local material = render_info and render_info:get_material() or 'materials/voxel.material.json'

         local blueprint = construction_data:get_blueprint_entity()
         model = blueprint:get_component('destination')
                              :get_region()
                                 :get()
                                    :intersect_region(stencil)

         render_node = _radiant.client.create_voxel_node(parent_node, model, material, MODEL_OFFSET)
      end
   end
   return render_node
end

return voxel_brush_util
