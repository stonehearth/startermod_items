local BuildEditor = class()
-- xxx: move all the proxy stuff to the client! - tony
local StructureEditor = require 'services.server.build.structure_editor'
local FloorEditor = require 'services.server.build.floor_editor'
local FloorEraser = require 'services.server.build.floor_eraser'
local PortalEditor = require 'services.server.build.portal_editor'
local WallLoopEditor = require 'services.server.build.wall_loop_editor'
local DoodadPlacer = require 'services.server.build.doodad_placer'
local Point3 = _radiant.csg.Point3

local log = radiant.log.create_logger('build_editor')


function BuildEditor:__init()
   self._model = _radiant.client.create_datastore()
   self._model:set_controller(self)
   _radiant.call('stonehearth:get_service', 'build')
      :done(function(r)
            -- we'd like to have just received the address instead of needing
            -- to manually convert it here, but even if the address was sent, 
            -- the client will try to convert it back to a DataStore before handing
            -- it to us =(
            self._build_service = r.result:__tojson()
         end)
end
function BuildEditor:add_doodad(session, response, uri)
   DoodadPlacer(self._build_service)
         :go(session, response, uri)
end

function BuildEditor:get_model()
   return self._model
end

function BuildEditor:place_new_wall(session, response, columns_uri, walls_uri)
   WallLoopEditor(self._build_service)
         :go(columns_uri, walls_uri, response)
end

function BuildEditor:place_new_floor(session, response, brush_shape)
   FloorEditor(self._build_service)
         :go(response, brush_shape)
end

function BuildEditor:erase_floor(session, response, brush_shape)
   FloorEraser(self._build_service)
         :go(response)
end

function BuildEditor:grow_walls(session, response, building, columns_uri, walls_uri)   
   _radiant.call_obj(self._build_service, 'grow_walls', building, columns_uri, walls_uri)
      :done(function(r)
            response:resolve(r)
         end)
      :fail(function(r)
            response:reject(r)
         end)
end

function BuildEditor:grow_roof(session, response, building)
   _radiant.call_obj(self._build_service, 'grow_roof', building, 'stonehearth:wooden_peaked_roof')
      :done(function(r)
            response:resolve(r)
         end)
      :fail(function(r)
            response:reject(r)
         end)
end

function BuildEditor:create_room(session, response)
   response:fail({ error = 'remove me!'})
end

return BuildEditor
