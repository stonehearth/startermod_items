local Entity = _radiant.om.Entity
local Point3 = _radiant.csg.Point3

local log = radiant.log.create_logger('portrait_renderer')

local PortraitRendererService = class()

function PortraitRendererService:initialize()
   self._created_entities = {}
   self._existing_entities = {}
   self._lights = {}

   self._request_queue = {}   

   _radiant.client.set_route_handler('get_portrait', function(uri, form, response)
         self:_render_portrait(form, response)
      end)
end

function PortraitRendererService:_render_portrait(request, response)
   assert(request)
   assert(response)
   
   table.insert(self._request_queue, {
         request = request,
         response = response,
      })
   if not self._pending_request then
      self:_pump_request_queue()
   end
end

function PortraitRendererService:_pump_request_queue()
   assert(not self._pending_request)
   self._pending_request = table.remove(self._request_queue, 1)
   if not self._pending_request then
      return
   end
   local request, response = self._pending_request.request, self._pending_request.response
   assert(request)
   assert(response)

   self:_stage_scene(request)
   _radiant.client.get_portrait(function(bytes)
         self:_clear_scene()
         self._pending_request = nil
         response:resolve_with_content(bytes, 'image/png');
         self:_pump_request_queue()
      end)
end

function PortraitRendererService:_create_entity(entity_uri, position, rotation)
   local ent = radiant.entities.create_entity(entity_uri)
   ent:add_component('render_info')
   local render_ent = _radiant.client.create_render_entity(_radiant.renderer.get_root_node(2), ent)

   table.insert(self._created_entities, {
      entity = ent,
      render_entity = render_ent,
   })
end

function PortraitRendererService:_add_existing_entity(ent)
   ent:add_component('render_info')
   local render_ent = _radiant.client.create_unmanaged_render_entity(_radiant.renderer.get_root_node(2), ent)

   table.insert(self._existing_entities, {
      render_entity = render_ent,
      entity = ent,
   })
   return render_ent
end

function PortraitRendererService:_set_camera_position(position)
   _radiant.renderer.portrait.camera.set_position(position)
end

function PortraitRendererService:_set_camera_look_at(target)
   _radiant.renderer.portrait.camera.look_at(target)
end

function PortraitRendererService:_add_light(light)
   local node = h3dAddLightNode(_radiant.renderer.get_root_node(2), "newlight", "DIRECTIONAL_LIGHTING", "DIRECTIONAL_SHADOWMAP", true)
   h3dSetNodeParamF(node, H3DLight.Radius2F, 0, 10000)
   h3dSetNodeParamF(node, H3DLight.FovF, 0, 360)
   h3dSetNodeParamI(node, H3DLight.ShadowMapCountI, 4)
   h3dSetNodeParamF(node, H3DLight.ShadowSplitLambdaF, 0, 0.95)
   h3dSetNodeParamF(node, H3DLight.ShadowMapBiasF, 0, 0.001)

   h3dSetNodeParamF(node, H3DLight.ColorF3, 0, light.color.x)
   h3dSetNodeParamF(node, H3DLight.ColorF3, 1, light.color.y)
   h3dSetNodeParamF(node, H3DLight.ColorF3, 2, light.color.z)

   h3dSetNodeParamF(node, H3DLight.AmbientColorF3, 0, light.ambient_color.x)
   h3dSetNodeParamF(node, H3DLight.AmbientColorF3, 1, light.ambient_color.y)
   h3dSetNodeParamF(node, H3DLight.AmbientColorF3, 2, light.ambient_color.z)
   h3dSetNodeTransform(node, 0, 0, 0, light.direction.x, light.direction.y, light.direction.z, 1, 1, 1)

   table.insert(self._lights, {
      node = node
   })
end

local function to_point3(p3_array)
   return Point3(p3_array[1], p3_array[2], p3_array[3])
end

function PortraitRendererService:_stage_scene(request)
   if request.type == 'headshot' then
      local entity = request.entity
      if radiant.util.is_a(entity, Entity) and entity:is_valid() then
         local render_entity = self:_add_existing_entity(entity)
         if request.animation then
            render_entity:pose(request.animation, request.time or 0)
         end
      else
        return -- Not an entity?
      end
      
      local entity_location = radiant.entities.get_world_grid_location(entity)

      -- Calculate the height of the camera based on the head bone of the entity.
      local render_info = entity:get_component('render_info')
      local camera_pos_y = 2.4
      if render_info then
         local animation_table_location = render_info:get_animation_table()
         local animation_table = radiant.resources.load_json(animation_table_location)
         local headPos = animation_table.skeleton.head
         local scale = render_info:get_scale()
         camera_pos_y = headPos[2] * scale
         camera_pos_y = camera_pos_y * 1.8
      end

      self:_add_light({
            color =         Point3(0.9, 0.8, 0.9),
            ambient_color = Point3(0.5,  0.5, 0.5),
            -- Direction is in degrees with yaw and pitch as the first 2 params. Ignore 3rd param
            -- -180 yaw will have light going from -z to positive z
            direction =     Point3(-160, 15, 0)
         })
         
      local mob = entity:get_component('mob')
      local rotation = mob and mob:get_facing()
      local camera_original_pos = Point3(1.7, camera_pos_y, -2.7)
      local camera_pos = radiant.math.rotate_about_y_axis(camera_original_pos, rotation)

      self:_set_camera_position(camera_pos + entity_location)
      self:_set_camera_look_at(Point3(0, camera_pos_y - 0.3, 0) + entity_location)
   end
end


function PortraitRendererService:_clear_scene()
   for _, light in pairs(self._lights) do
      h3dRemoveNode(light.node)
   end
   self._lights = {}

   for _, entity_data in pairs(self._created_entities) do
      radiant.entities.destroy_entity(entity_data.entity)
   end
   self._created_entities = {}

   for _, entity_data in pairs(self._existing_entities) do
      entity_data.render_entity:destroy()
      entity_data.render_entity = nil
   end
   self._existing_entities = {}
end

return PortraitRendererService
