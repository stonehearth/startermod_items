local BuildEditor = class()
-- xxx: move all the proxy stuff to the client! - tony
local ProxyWall = require 'services.server.build.proxy_wall'
local ProxyWallBuilder = require 'services.server.build.proxy_wall_builder'
local ProxyFloorBuilder = require 'services.server.build.proxy_floor_builder'
local ProxyRoomBuilder = require 'services.server.build.proxy_room_builder'
local Point3 = _radiant.csg.Point3

local function get_building_for(entity)
   if entity then
      local cp = entity:get_component('stonehearth:construction_progress')
      if cp then
         return cp:get_data().building_entity
      end
      local fab = entity:get_component('stonehearth:fabricator')
      if fab then
         return get_building_for(fab:get_data().blueprint)
      end
   end
end

function BuildEditor:__init()
   self._model = _radiant.client.create_datastore()
   self._model:set_controller(self)
end

function BuildEditor:get_model()
   return self._model
end

function BuildEditor:place_new_wall(session, response)
   ProxyWallBuilder():go(response)
end

function BuildEditor:place_new_floor(session, response)
   ProxyFloorBuilder():go(response)
end

function BuildEditor:grow_walls(session, response)
   local capture = stonehearth.input:capture_input()
   capture:on_mouse_event(function(e)
         if e:up(1) then
            local s = _radiant.client.query_scene(e.x, e.y)
            if s then
               local entity = radiant.get_object(s:objectid_of(0))
               if entity and entity:is_valid() then
                  local building = get_building_for(entity)
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

function BuildEditor:create_room(session, response)
   ProxyRoomBuilder():go()  
   return true
end

return BuildEditor
