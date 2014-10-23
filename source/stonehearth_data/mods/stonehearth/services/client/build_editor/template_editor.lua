local constants = require('constants').construction
local build_util = require 'lib.build_util'
local Cube3 = _radiant.csg.Cube3
local Point3 = _radiant.csg.Point3
local Point3 = _radiant.csg.Point3
local Region3 = _radiant.csg.Region3
local Quaternion = _radiant.csg.Quaternion

local OFFSCREEN = Point3(0, -100000, 0)

local TemplateEditor = class()

function TemplateEditor:__init(build_service)
   self._log = radiant.log.create_logger('builder')
   self._build_service = build_service
end

function TemplateEditor:_restore_template(template_name)
   local template = radiant.mods.read_object('building_templates/' .. template_name)
   if template then
      self._building = radiant.entities.create_entity('stonehearth:entities:building')
      build_util.restore_template(self._building, template_name, { mode = 'preview'})

      local bounds = build_util.get_building_bounds(self._building)
      self._center_offset = Point3(bounds.max.x / 2, 0, bounds.max.z / 2):to_int();
      radiant.entities.move_to(self._building, -self._center_offset)

      radiant._authoring_root_entity:add_component('entity_container'):add_child(self._building)
      self._render_entity = _radiant.client.create_group_node(1)
      self._building_render_entity = _radiant.client.create_render_entity(self._render_entity:get_node(), self._building)
   end
end

function TemplateEditor:go(response, template_name)
   self:_restore_template(template_name)

   stonehearth.selection:select_location()
      :set_cursor('stonehearth:cursors:place_template')
      :set_filter_fn(function (result)
            self._log:spam('testing: %s', result.entity)
            if result.entity:get_id() ~= 1 then
               -- not right, but unblocks me
               return stonehearth.selection.FILTER_IGNORE
            end

            -- ignore transparent entities (e.g. the no construction zone at the base of buildings)
            local rcs = result.entity:get_component('region_collision_shape')
            if rcs and rcs:get_region_collision_type() == _radiant.om.RegionCollisionShape.NONE then
               return stonehearth.selection.FILTER_IGNORE
            end

            return true
         end)
      :progress(function(selector, location, rotation)
            if location then
               self._render_entity:set_position(location)
               self._render_entity:set_rotation(Point3(0, rotation, 0))               
            else
               self._render_entity:set_position(OFFSCREEN)
            end
         end)
      :done(function(selector, location, rotation)
            _radiant.call_obj(self._build_service, 'build_template_command', template_name, location, self._center_offset, rotation)
               :done(function(r)
                     if r.new_building then
                        stonehearth.selection:select_entity(r.new_building)
                     end
                     response:resolve(r)
                  end)
               :fail(function(r)
                     response:reject(r)
                  end)
               :always(function()
                     self:destroy()
                     selector:destroy()
                  end)            
            response:resolve({})
         end)
      :fail(function(selector)
            response:reject({})
            self:destroy()
            selector:destroy()
         end)
      :go()
end

function TemplateEditor:destroy()
   if self._building then
      radiant.entities.destroy_entity(self._building)
      self._building = nil
   end
   self._render_entity = nil
end

return TemplateEditor 
