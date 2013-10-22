local BuildEditor = class()
local ProxyWall = require 'services.build.proxy_wall'
local ProxyWallBuilder = require 'services.build.proxy_wall_builder'
local ProxyRoomBuilder = require 'services.build.proxy_room_builder'
local Point3 = _radiant.csg.Point3

function BuildEditor:__init()
   self._model = _radiant.client.create_data_store()
   self._model:set_controller(self)
end

function BuildEditor:get_model()
   return self._model
end

function BuildEditor:place_new_wall(session, response, wall_uri)
   ProxyWallBuilder()
      :set_wall_uri(wall_uri)
      :set_column_uri('stonehearth:wooden_column')
      :go()
   
   return { success=true }
end

function BuildEditor:create_room(session, response, wall_uri)
   ProxyRoomBuilder()
      :set_wall_uri(wall_uri)
      :set_column_uri('stonehearth:wooden_column')
      :go()
   
   return { success=true }
end

return BuildEditor
