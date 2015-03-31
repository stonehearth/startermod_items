local Point3 = _radiant.csg.Point3

local log = radiant.log.create_logger('portrait_renderer')

local PortraitRendererService = class()

function PortraitRendererService:initialize()
   self._entities = {}
   self._lights = {}
end


function PortraitRendererService:_add_entity(entity_data, position, rotation)
   local ent = radiant.entities.create_entity(entity_data.uri)
   ent:add_component('render_info')--:set_material('materials/ghost_item.json')
   local render_ent = _radiant.client.create_render_entity(_radiant.renderer.get_root_node(2), ent)

   table.insert(self._entities, {
      entity = ent,
      render_entity = render_ent,
   })
end

function PortraitRendererService:_set_camera_position(position)
   _radiant.renderer.portrait.camera.set_position(position)
end

function PortraitRendererService:_set_camera_look_at(target)
   _radiant.renderer.portrait.camera.look_at(target)
end

function PortraitRendererService:_add_light(position, color, radius)
   local node = h3dAddLightNode(_radiant.renderer.get_root_node(2), "newlight", "DIRECTIONAL_LIGHTING", "DIRECTIONAL_SHADOWMAP", true)
   h3dSetNodeTransform(node,  0, 0, 0, 0, 0, 0, 1, 1, 1)
   h3dSetNodeParamF(node, H3DLight.Radius2F, 0, 10000)
   h3dSetNodeParamF(node, H3DLight.FovF, 0, 360)
   h3dSetNodeParamI(node, H3DLight.ShadowMapCountI, 4)
   h3dSetNodeParamF(node, H3DLight.ShadowSplitLambdaF, 0, 0.95)
   h3dSetNodeParamF(node, H3DLight.ShadowMapBiasF, 0, 0.001)

   h3dSetNodeParamF(node, H3DLight.ColorF3, 0, 1)
   h3dSetNodeParamF(node, H3DLight.ColorF3, 1, 1)
   h3dSetNodeParamF(node, H3DLight.ColorF3, 2, 1)

   h3dSetNodeParamF(node, H3DLight.AmbientColorF3, 0, 0.1)
   h3dSetNodeParamF(node, H3DLight.AmbientColorF3, 1, 0.1)
   h3dSetNodeParamF(node, H3DLight.AmbientColorF3, 2, 0.1)

   table.insert(self._lights, {
      node = node
   })
end

--[[

{
   lights : [
      {
         color:Color3,
         position:Point3,
         radius:float
      },
   ],
   entities: [
      {
         uri: string,
         position: Point3,
         rotation: Quat
         // Probably want an animation/frame here, too.
      }
   ],

   camera: {
      position: Point3,
      look_at: Point3
   }
}


]]


function _to_point3(p3_array)
   return Point3(p3_array[1], p3_array[2], p3_array[3])
end

function _parse_scene(scene_json)
   local scene = {}

   local lights = {}
   for _, light in pairs(scene_json['lights']) do
      table.insert(lights, {
         color = _to_point3(light['color']),
         position = _to_point3(light['position']),
         radius = light['radius'],
      })
   end
   scene['lights'] = lights

   local entities = {}
   for _, entity in pairs(scene_json['entities']) do
      table.insert(entities, {
         uri = entity['uri'],
         position = _to_point3(entity['position']),
         --rotation = entity['rotation']),
      })
   end
   scene['entities'] = entities

   scene['camera'] = {
      position = _to_point3(scene_json['camera']['position']),
      look_at = _to_point3(scene_json['camera']['look_at']),
   }

   return scene
end

function PortraitRendererService:set_scene(scene_json)
   local scene = _parse_scene(scene_json)

   for _, light in pairs(scene['lights']) do
      self:_add_light(light)
   end

   for _, entity in pairs(scene['entities']) do
      self:_add_entity(entity)
   end

   self:_set_camera_position(scene['camera'].position)
   self:_set_camera_look_at(scene['camera'].look_at)
end

function PortraitRendererService:clear_scene()

end


function PortraitRendererService:render_scene(session, response, scene_json)
   self:clear_scene()
   self:set_scene(scene_json)

   local render_count = 0
   local render_trace
   render_trace = _radiant.client.trace_render_frame()
                                    :on_frame_finished('portrait render wait', function(now, interpolate)
                                             render_count = render_count + 1
                                             if render_count > 10 then
                                                render_trace:destroy()
                                                _radiant.call('radiant:client:get_portrait'):done(function(resp)
                                                      response:resolve(resp)
                                                   end)
                                             end
                                       end)
end

return PortraitRendererService
