local BuildEditor = class()
-- xxx: move all the proxy stuff to the client! - tony
local StructureEditor = require 'services.server.build.structure_editor'
local WallEditor = require 'services.server.build.wall_editor'
local ProxyWall = require 'services.server.build.proxy_wall'
local ProxyWallBuilder = require 'services.server.build.proxy_wall_builder'
local ProxyFloorBuilder = require 'services.server.build.proxy_floor_builder'
local ProxyRoomBuilder = require 'services.server.build.proxy_room_builder'
local Point3 = _radiant.csg.Point3

local log = radiant.log.create_logger('build_editor')

function BuildEditor:add_door(session, response)
   local wall_editor
   local capture = stonehearth.input:capture_input()

   capture:on_mouse_event(function(e)
         local s = _radiant.client.query_scene(e.x, e.y)
         if s and s:get_result_count() > 0 then            
            local entity = s:get_result(0).entity
            if entity and entity:is_valid() then
               log:detail('hit entity %s', entity)
               if wall_editor and not wall_editor:should_keep_focus(entity) then
                  log:detail('destroying wall editor')
                  wall_editor:destroy()
                  wall_editor = nil
               end
               if not wall_editor then
                  local fabricator, blueprint, project = StructureEditor.get_fbp_for_structure(entity, 'stonehearth:wall')
                  log:detail('got blueprint %s', tostring(blueprint))
                  if blueprint then
                     log:detail('creating wall editor for blueprint: %s', blueprint)
                     wall_editor = WallEditor()
                                       :begin_editing(fabricator, blueprint, project)
                                       :set_portal_uri('stonehearth:wooden_door')
                  end
               end
               if wall_editor then
                  wall_editor:on_mouse_event(e, s)
               end
            end
         end
         if e:up(1) then
            if wall_editor then
               wall_editor:submit(response)
            end
            capture:destroy()
         end
         return true
      end) 
end

function BuildEditor:__init()
   self._model = _radiant.client.create_datastore()
   self._model:set_controller(self)
end

function BuildEditor:get_model()
   return self._model
end

function BuildEditor:place_new_wall(session, response)
   ProxyWallBuilder()
         :go('stonehearth:wooden_column', 'stonehearth:wooden_wall', response)
end

function BuildEditor:place_new_floor(session, response)
   ProxyFloorBuilder():go(response)
end

function BuildEditor:grow_walls(session, response)
   local capture = stonehearth.input:capture_input()
   capture:on_mouse_event(function(e)
         if e:up(1) then
            local s = _radiant.client.query_scene(e.x, e.y)
            if s and s:get_result_count() > 0 then
               local entity = s:get_result(0).entity
               if entity and entity:is_valid() then
                  local building = StructureEditor.get_building_for(entity)
                  if building then
                     _radiant.call('stonehearth:grow_walls', building, 'stonehearth:wooden_column', 'stonehearth:wooden_wall')
                        :done(function(r)
                              response:resolve(r)
                           end)
                        :fail(function(r)
                              response:reject(r)
                           end)
                     capture:destroy()
                     return
                  end
               end
            end
            response:reject({ error = 'unknown error' })
            capture:destroy()
         end
         return true
      end)   
end

function BuildEditor:grow_roof(session, response)
   local capture = stonehearth.input:capture_input()
   capture:on_mouse_event(function(e)
         if e:up(1) then
            local s = _radiant.client.query_scene(e.x, e.y)
            if s and s:get_result_count() > 0 then            
               local entity = s:get_result(0).entity
               if entity and entity:is_valid() then
                  local building = StructureEditor.get_building_for(entity)
                  if building then
                     _radiant.call('stonehearth:grow_roof', building, 'stonehearth:wooden_peaked_roof')
                        :done(function(r)
                              response:resolve(r)
                           end)
                        :fail(function(r)
                              response:reject(r)
                           end)
                     capture:destroy()
                     return
                  end
               end
            end
            response:reject({ error = 'unknown error' })
            capture:destroy()
         end
         return true
      end)   
end

function BuildEditor:create_room(session, response)
   ProxyRoomBuilder():go()  
   return true
end

return BuildEditor
