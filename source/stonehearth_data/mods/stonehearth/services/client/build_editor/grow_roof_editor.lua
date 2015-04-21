local build_util = require 'lib.build_util'
local constants = require('constants').construction
local StructureEditor = require 'services.client.build_editor.structure_editor'

local GrowRoofEditor = class()
local log = radiant.log.create_logger('build_editor.grow_roof')

-- this is the component which manages the fabricator entity.
function GrowRoofEditor:__init(build_service)
   log:debug('created')

   self._last_target = nil
   self._build_service = build_service
end

function GrowRoofEditor:destroy()
   log:debug('destroyed')
   self._last_target = nil
   self:_destroy_preview_entities()
end

function GrowRoofEditor:_destroy_preview_entities()
   if self._preview_roof then
      self._preview_roof:destroy()
      self._preview_roof = nil
   end
end

function GrowRoofEditor:apply_options(roof_options)
   self._roof_options = roof_options
   if self._last_target then
      local target = self._last_target
      self._last_target = nil
      self:_switch_to_target()
   end
end

function GrowRoofEditor:go(response, roof_options)
   log:detail('running')

   self._roof_options = roof_options
   stonehearth.selection:select_entity_tool()
      :set_cursor('stonehearth:cursors:grow_roof')
      :set_filter_fn(function(result)
            local entity = result.entity
            if radiant.entities.is_temporary_entity(entity) then
               -- one of our client-side preview entities.  bail.
               return stonehearth.selection.FILTER_IGNORE
            end
            local blueprint = build_util.get_blueprint_for(entity)
            if not blueprint then
               -- not a blueprint, bail.
               return false
            end
            local building = build_util.get_building_for(blueprint)
            if not building then
               -- no building for this blueprint.  weird!  bail.
               return false
            end
            local wall = blueprint:get_component('stonehearth:wall')
            if not wall then
               -- only grow around floor
               return false
            end
            return true
         end)
      :progress(function(selector, entity)
            self:_switch_to_target(entity)
         end)
      :done(function(selector, entity)
            if entity then
               _radiant.call_obj(self._build_service, 'grow_roof_command', self._last_target, self._roof_options)
                  :done(function(r)
                        if r.new_selection then
                           stonehearth.selection:select_entity(r.new_selection)
                        end
                     end)
                  :always(function()
                        self:destroy()
                     end)
            else
               self:destroy()
            end
            response:resolve('done')
         end)
      :fail(function(selector)
            log:detail('failed to select building')
            response:reject('failed')
            self:destroy()
         end)
      :go()

   return self
end

function GrowRoofEditor:_switch_to_target(target)
   if target ~= self._last_target then
      self._last_target = target
      self:_destroy_preview_entities()
      if target then
         local world_origin, region2 = build_util.calculate_roof_shape_around_walls(target, self._roof_options)
         if region2 then
            self:_create_preview_roof(world_origin, region2)
         end
      end
   end
end

function GrowRoofEditor:_create_preview_roof(world_origin, region2)
   local editor = StructureEditor()
   editor:create_blueprint('stonehearth:build:prototypes:roof', 'stonehearth:roof')
   
   local roof = editor:get_proxy_blueprint()
   roof:add_component('stonehearth:roof')
            :apply_nine_grid_options(self._roof_options)
            :cover_region2(self._roof_options.brush, region2)
            :layout()

   -- sigh.  i hate that we have to do this...
   editor:move_to(world_origin)

   self._preview_roof = editor
end

return GrowRoofEditor

