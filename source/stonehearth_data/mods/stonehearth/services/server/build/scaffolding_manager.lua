local build_util = require 'stonehearth.lib.build_util'

local Point2  = _radiant.csg.Point2
local Point3  = _radiant.csg.Point3
local Cube3   = _radiant.csg.Cube3
local Region3 = _radiant.csg.Region3

local INFINITE = 1000000
local CLIP_SOLID = _radiant.physics.Physics.CLIP_SOLID

local log = radiant.log.create_logger('build.scaffolding')
local ScaffoldingManager = class()

function ScaffoldingManager:initialize()
   self._sv.next_id = 2
   self._sv.builders = {}        -- line and plen builders
   self._sv.regions = {}         -- per builder..
   self._sv.scaffolding = {}

   self._new_regions = {}
   self._changed_scaffolding = {}
   self._region_traces = {}

   self.__saved_variables:mark_changed()
end

function ScaffoldingManager:activate()
end

function ScaffoldingManager:request_scaffolding_for(requestor, blueprint_rgn, project_rgn, normal, stand_at_base)
   checks('self', 'Entity', 'Region3Boxed', 'Region3Boxed', '?Point3', 'boolean')

   local size = blueprint_rgn:get():get_bounds():get_size()
   local builder_type
   if size.x == 1 or size.z == 1 then
      builder_type = 'stonehearth:build:scaffolding_builder_1d'
   else
      builder_type = 'stonehearth:build:scaffolding_builder_2d'
   end
   return self:_create_builder(builder_type, requestor, blueprint_rgn, project_rgn, normal, stand_at_base)
end

function ScaffoldingManager:_get_next_id()
   local id = self._sv.next_id 
   self._sv.next_id = self._sv.next_id + 1
   self.__saved_variables:mark_changed()
   return id
end

function ScaffoldingManager:_create_builder(builder_type, requestor, blueprint_rgn, project_rgn, normal, stand_at_base)
   checks('self', 'string', 'Entity', 'Region3Boxed', 'Region3Boxed', '?Point3', 'boolean')
   
   local rid = self:_get_next_id()
   local builder = radiant.create_controller(builder_type, self, rid, requestor, blueprint_rgn, project_rgn, normal, stand_at_base)
   
   self._sv.builders[rid] = builder
   self.__saved_variables:mark_changed()
   
   return builder
end

function ScaffoldingManager:_add_region(rid, entity, origin, blueprint_region, region, normal)
   checks('self', 'number', 'Entity', 'Point3', 'Region3Boxed', 'Region3Boxed', 'Point3')

   assert(not self._sv.regions[rid])

   local rblock = {
      rid    = rid,
      entity = entity,
      origin = origin,
      region = region,
      normal = normal,
      blueprint_region = blueprint_region,
   }
   self._sv.regions[rid] = rblock
   self:_trace_region(rblock)
end

function ScaffoldingManager:_remove_region(rid)
   if self._sv.regions[rid] then
      assert(self._sv.regions[rid])
      self._sv.regions[rid] = nil
      self._region_traces[rid]:destroy()
      self._region_traces[rid] = nil
      radiant.not_yet_implemented()       -- gotta pull it out of the scaffolding, too
   end
end

function ScaffoldingManager:_trace_region(rblock)
   assert(rblock)
   assert(not self._region_traces[rblock.rid])

   local trace = rblock.region:trace('watch scaffolding regions')
                                 :on_changed(function()
                                       self:_mark_region_changed(rblock)
                                    end)

   self._region_traces[rblock.rid] = trace
   trace:push_object_state()
end

function ScaffoldingManager:_mark_region_changed(rblock)
   if not self._marked_changed then
      self._marked_changed = true
      radiant.events.listen_once(radiant, 'stonehearth:gameloop', self, self._process_changes)
   end
   local sblock = rblock.sblock
   if sblock then
      self._changed_scaffolding[sblock.sid] = sblock
   else
      self._new_regions[rblock.rid] = rblock
   end
end

function ScaffoldingManager:_process_changes()
   self._marked_changed = false
   self:_process_new_regions()
   self:_process_changed_scaffolding()
end

function ScaffoldingManager:_process_new_regions()
   for rid, rblock in pairs(self._new_regions) do
      assert(not rblock.sblock)

      if not self:_find_scaffolding_for(rblock) then
         self:_create_scaffolding_for(rblock)
      end
      local sblock = rblock.sblock
      assert(sblock)
      assert(sblock.regions[rblock.rid] == rblock)

      self._changed_scaffolding[sblock.sid] = sblock
   end
   self._new_regions = {}
end

function ScaffoldingManager:_process_changed_scaffolding()
   for sid, sblock in pairs(self._changed_scaffolding) do
      self:_update_scaffolding_region(sblock)
   end
   self._changed_scaffolding = {}
end

function ScaffoldingManager:_find_scaffolding_for(rblock)
   local origin = rblock.origin
   local region = rblock.blueprint_region:get()

   -- look "down" for someone who can help us out.
   local zone = region:translated(origin + rblock.normal)
   zone = _physics:project_region(zone, CLIP_SOLID)

   local sblock
   radiant.terrain.get_entities_in_region(zone, function(entity)
         if not sblock then
            local fabricator, _, _ = build_util.get_fbp_for(entity)
            if fabricator then
               for _, sblk in pairs(self._sv.scaffolding) do
                  if sblk.fabricator == fabricator then
                     sblock = sblk
                     break
                  end
               end
            end
         end
      end)

   if not sblock then
      return false
   end
   rblock.sblock = sblock
   sblock.regions[rblock.rid] = rblock
   return true
end

function ScaffoldingManager:_create_scaffolding_for(rblock)
   checks('self', 'table')
   assert(not rblock.sblock)

   local owner = radiant.entities.get_player_id(rblock.entity)
   local origin = rblock.origin
   local normal = rblock.normal

   local region = radiant.alloc_region3()
   local scaffolding = radiant.entities.create_entity('stonehearth:build:prototypes:scaffolding', { owner = owner })

   scaffolding:add_component('stonehearth:construction_data')
                  :set_normal(normal)
   scaffolding:add_component('destination')
                  :set_region(region)

   -- create a fabricator entity to build the scaffolding
   local fabricator = radiant.entities.create_entity('', { owner = owner })
   radiant.terrain.place_entity_at_exact_location(fabricator, origin)

   fabricator:set_debug_text('(Fabricator for ' .. tostring(scaffolding) .. ')')   
   fabricator:add_component('stonehearth:fabricator')
                              :start_project(scaffolding)

   build_util.bind_fabricator_to_blueprint(scaffolding, fabricator, 'stonehearth:fabricator')

   -- let's go go go!
   stonehearth.build:set_active(scaffolding, true)

   local sid = self:_get_next_id()
   local sblock = {
      sid         = sid,
      scaffolding = scaffolding,
      fabricator  = fabricator,      
      origin      = origin,
      region      = region,
      owner       = owner,
      normal      = normal,
      regions     = { [rblock.rid] = rblock },
      ladder_builders = { },
   }
   self._sv.scaffolding[sid] = sblock
   rblock.sblock = sblock

   return sblock
end

function ScaffoldingManager:_update_scaffolding_region(sblock)
   local merged = Region3()
   for rid, rblock in pairs(sblock.regions) do
      local r = rblock.region:get():translated(rblock.origin)
      merged:add_region(r)
   end   
   merged:translate(-sblock.origin)

   sblock.region:modify(function(cursor)
         cursor:copy_region(merged)
      end)

   local new_ladder_builders = {}
   for rid, rblock in pairs(sblock.regions) do
      local climb_to = self:_compute_ladder_top(rblock)
      if climb_to then
         local ladder_builder = stonehearth.build:request_ladder_to(sblock.owner,
                                                 climb_to,
                                                 rblock.normal)
         new_ladder_builders[rblock.rid] = ladder_builder
      end
   end
   for rid, ladder_builder in pairs(sblock.ladder_builders) do
      ladder_builder:destroy()
   end
   sblock.ladder_builders = new_ladder_builders
end

function ScaffoldingManager:_compute_ladder_top(rblock)
   local normal = rblock.normal
   local ladder_stencil = Region3(Cube3(Point3(0, -INFINITE, 0) + normal,
                                        Point3(1,  INFINITE, 1) + normal))
   local ladder = rblock.region:get():intersect_region(ladder_stencil)
   local ladder_bounds = ladder:get_bounds()
   local height = ladder_bounds.max.y - 1
   if height == 0 then
      -- do we need a ladder?  check the base!
      local block_under = ladder_bounds.min + normal + normal - Point3.unit_y
      if radiant.terrain.is_blocked(block_under) then
         -- yes, we think the ladder should be 0 high and there's actually something
         -- there to stand on.  bail!
         return
      end
   end

   local normal = rblock.normal
   local climb_to = rblock.origin + normal + normal
   climb_to.y = climb_to.y + height

   return climb_to
end

return ScaffoldingManager
