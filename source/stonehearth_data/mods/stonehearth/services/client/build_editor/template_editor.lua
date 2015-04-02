local constants = require('constants').construction
local build_util = require 'lib.build_util'
local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3
local Region3 = _radiant.csg.Region3
local Quaternion = _radiant.csg.Quaternion

local INFINITE = 100000
local OFFSCREEN = Point3(0, -INFINITE, 0)
local UNDERGROUND = Cube3(Point3(-INFINITE, -INFINITE, -INFINITE), Point3(INFINITE, 0, INFINITE))
local OVERGROUND  = Cube3(Point3(-INFINITE, 0, -INFINITE), Point3(INFINITE, INFINITE, INFINITE))

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

      self._center_offset = build_util.get_building_centroid(self._building)
      radiant.entities.move_to(self._building, -self._center_offset)

      radiant._authoring_root_entity:add_component('entity_container'):add_child(self._building)
      self._render_entity = _radiant.client.create_group_node(H3DRootNode)
      self._building_render_entity = _radiant.client.create_render_entity(self._render_entity:get_node(), self._building)

      -- create two regions for testing to see if the building can be placed here.  one includes
      -- all the points which need to be unblocked above the service.  the other includes all the
      -- points that must be solid below the surface (to support the building)
      local bc = self._building:get_component('stonehearth:building')
      local footprint = bc:get_building_footprint()

      self._surface_region = Region3(footprint)
      self._surface_region:subtract_cube(UNDERGROUND)

      self._underground_region = Region3(footprint)
      self._underground_region:subtract_cube(OVERGROUND)

      -- iterate through all the structures in this building, modifying the surface and
      -- underground regions as we go
      local structures = bc:get_all_structures()
      for _, entry in pairs(structures['stonehearth:floor']) do
         local floor = entry.entity
         local floor_region = floor:get_component('region_collision_shape')
                                       :get_region()
                                          :get()

         -- the slice directly above the floor must be empty.  this stops us from placing
         -- buildings where the tree grows up through the floor, for example.
         self._surface_region:add_region(floor_region:translated(Point3.unit_y))
      end

      -- translate both regions to the center of the placement cursor.
      self._surface_region:translate(-self._center_offset)
      self._underground_region:translate(-self._center_offset)
   end
end

function TemplateEditor:go(response, template_name)
   self:_restore_template(template_name)

   local last_rotation = 0
   local building = self._building
   stonehearth.selection:select_location()
      :set_cursor('stonehearth:cursors:place_template')
      :set_filter_fn(function (result, selector)
            local entity = result.entity
            if entity == building or radiant.entities.is_child_of(entity, building) then
               -- stab through the current cursor.  it's made up of a ton of entities,
               -- so make sure we ignore all of them.
               return stonehearth.selection.FILTER_IGNORE
            end

            local location = result.brick
            local rotation = selector:get_rotation()

            -- translate the surface region to the location of the cursor and make sure
            -- it doesn't overlap with any entities which would prevent us from building
            local surface_region = Region3()
            surface_region:copy_region(self._surface_region)
            surface_region:rotate(rotation)
            surface_region:translate(location)

            local overlapping = radiant.terrain.get_entities_in_region(surface_region)
            for _, overlap in pairs(overlapping) do
               if radiant.entities.is_solid_entity(overlap) then
                  return stonehearth.selection.FILTER_IGNORE
               end
               if radiant.entities.get_entity_data(overlap, 'stonehearth:designation') then
                  return stonehearth.selection.FILTER_IGNORE
               end
            end

            -- make sure every point in the underground region is solid.  this prevents
            -- us from having buildings floating in the air (at least at placement time!)
            local underground_region = Region3()
            underground_region:copy_region(self._underground_region)
            underground_region:rotate(rotation)
            underground_region:translate(location)

            for pt in underground_region:each_point() do
               if not radiant.terrain.is_blocked(pt) then
                  return stonehearth.selection.FILTER_IGNORE
               end
            end

            -- looks good!  let's put it here
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
