local BuildEditor = class()
-- xxx: move all the proxy stuff to the client! - tony
local StructureEditor = require 'services.server.build.structure_editor'
local FloorEditor = require 'services.server.build.floor_editor'
local PortalEditor = require 'services.server.build.portal_editor'
local WallLoopEditor = require 'services.server.build.wall_loop_editor'
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
                     wall_editor = PortalEditor()
                                       :begin_editing(fabricator, blueprint, project)
                                       :set_portal_uri('stonehearth:wooden_door')
                                       :go()
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
   WallLoopEditor()
         :go('stonehearth:wooden_column', 'stonehearth:wooden_wall', response)
end

function BuildEditor:place_new_floor(session, response)
   FloorEditor():go(response)
end

function BuildEditor:grow_walls(session, response, building)
   _radiant.call('stonehearth:grow_walls', building, 'stonehearth:wooden_column', 'stonehearth:wooden_wall')
      :done(function(r)
            response:resolve(r)
         end)
      :fail(function(r)
            response:reject(r)
         end)
end

function BuildEditor:grow_roof(session, response, building)
   _radiant.call('stonehearth:grow_roof', building, 'stonehearth:wooden_peaked_roof')
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
