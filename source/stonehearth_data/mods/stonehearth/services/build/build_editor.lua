local BuildEditor = class()
local ProxyWall = require 'services.build.proxy_wall'
local ProxyWallBuilder = require 'services.build.proxy_wall_builder'
local ProxyRoomBuilder = require 'services.build.proxy_room_builder'
local Point3 = _radiant.csg.Point3

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

function BuildEditor:create_room(session, response)
   ProxyRoomBuilder():go()  
   return true
end

return BuildEditor
