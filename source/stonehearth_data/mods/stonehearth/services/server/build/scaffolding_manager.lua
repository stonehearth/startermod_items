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
   self.__saved_variables:mark_changed()
end

function ScaffoldingManager:activate()
   self._new_regions = {}
   self._changed_scaffolding = {}
   self._rblock_region_traces = {}
   self._sblock_region_traces = {}

   for rid, rblock in pairs(self._sv.regions) do
      self:_trace_rblock_region(rblock)
   end
   for sid, sblock in pairs(self._sv.scaffolding) do
      self:_trace_sblock_region(sblock)
   end
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

   log:detail('creating %s for %s (rid:%d)', builder_type, requestor, rid)
   local builder = radiant.create_controller(builder_type, self, rid, requestor, blueprint_rgn, project_rgn, normal, stand_at_base)
   
   self._sv.builders[rid] = builder
   self.__saved_variables:mark_changed()
   
   return builder
end

function ScaffoldingManager:_remove_builder(bid)
   self:_remove_region(bid)

   local builder = self._sv.builders[bid]
   if builder then
      self._sv.builders[bid] = nil
   end
   self.__saved_variables:mark_changed()
end

function ScaffoldingManager:_add_region(rid, debug_text, entity, origin, blueprint_region, blueprint_clip_box, region, normal)
   checks('self', 'number', 'string', 'Entity', 'Point3', 'Region3Boxed', '?Cube3', 'Region3Boxed', 'Point3')

   assert(not self._sv.regions[rid])

   local rblock = {
      rid    = rid,
      entity = entity,
      origin = origin,
      region = region,
      normal = normal,
      debug_text = debug_text,
      blueprint_region = blueprint_region,
      blueprint_clip_box = blueprint_clip_box,
   }
   self._sv.regions[rid] = rblock
   log:detail('adding region rid:%d origin:%s dbg:%s blueprint-bounds:%s clip_box:%s', rid, origin, debug_text, blueprint_region:get():get_bounds(), tostring(blueprint_clip_box))
   self:_trace_rblock_region(rblock)
end

function ScaffoldingManager:_remove_region(rid)
   local rblock = self._sv.regions[rid]

   log:detail('removing region rid:%d', rid)
   if rblock then
      -- remove all the rblock tracking data
      assert(self._sv.regions[rid])
      self._sv.regions[rid] = nil

      if self._rblock_region_traces[rid] then
         log:spam('destroying rblock rid:%d region trace', rblock.rid)
         self._rblock_region_traces[rid]:destroy()
         self._rblock_region_traces[rid] = nil
      end

      -- remove the rblock from the sblock
      local sblock = rblock.sblock
      if sblock then
         local sid = sblock.sid
         log:detail('removing region rid:%d from sblock:%d', rid, sid)
         sblock.regions[rid] = nil
         self._changed_scaffolding[sid] = sblock
         self:_mark_changed()
      end
   end
end

function ScaffoldingManager:_trace_sblock_region(sblock)
   assert(sblock)
   assert(not self._sblock_region_traces[sblock.sid])

   local trace = sblock.region:trace('watch scaffolding sblock regions')
                                 :on_changed(function()
                                       self:_check_sblock_destroy(sblock)
                                    end)

   self._sblock_region_traces[sblock.sid] = trace
   trace:push_object_state()
end

function ScaffoldingManager:_trace_rblock_region(rblock)
   assert(rblock)
   assert(not self._rblock_region_traces[rblock.rid])

   log:spam('tracing rblock rid:%d region', rblock.rid)
   local trace = rblock.region:trace('watch scaffolding rblock regions')
                                 :on_changed(function()
                                       self:_mark_rblock_region_changed(rblock)
                                    end)

   self._rblock_region_traces[rblock.rid] = trace
   trace:push_object_state()
end

function ScaffoldingManager:_check_sblock_destroy(sblock)
   local sid = sblock.sid

   log:detail('checking sblock sid:%d region.', sid)

   if not radiant.empty(sblock.regions) then
      if log:is_enabled(radiant.log.DETAIL) then
         local regions = ''
         for rid, rblock in pairs(sblock.regions) do
            regions = tostring(rid) .. ' '
         end
         log:detail('sblock sid:%d still has regions %s.  not destroying.', sid, regions)
      end
      return
   end
   if not sblock.region:get():empty() then
      log:detail('sblock sid:%d still has a non-empty region.  not destroying.', sblock.sid)
      return
   end

   log:detail('reaping sblock sid:%d', sid)
   if self._sblock_region_traces[sid]  then
      self._sblock_region_traces[sid]:destroy()
      self._sblock_region_traces[sid] = nil
   end
   self._sv.scaffolding[sid] = nil
   self.__saved_variables:mark_changed()

   radiant.entities.destroy_entity(sblock.scaffolding)
   sblock.region = nil
end

function ScaffoldingManager:_mark_changed()
   if not self._marked_changed then
      self._marked_changed = true
      radiant.events.listen_once(radiant, 'stonehearth:gameloop', self, self._process_changes)
   end
end

function ScaffoldingManager:_mark_rblock_region_changed(rblock)
   self:_mark_changed()
   local sblock = rblock.sblock
   if sblock then
      log:detail('rid:%d changed.  marking sblock sid:%d changed as well.', rblock.rid, sblock.sid)
      self._changed_scaffolding[sblock.sid] = sblock
   else
      log:detail('rid:%d changed.  no sblock yet.  adding to new region set!', rblock.rid)
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
      if rblock.entity:is_valid() then
         if not self:_find_scaffolding_for(rblock) then
            self:_create_scaffolding_for(rblock)
         end
         local sblock = rblock.sblock
         assert(sblock)
         assert(sblock.regions[rblock.rid] == rblock)

         self._changed_scaffolding[sblock.sid] = sblock
      end
   end
   self._new_regions = {}
end

function ScaffoldingManager:_process_changed_scaffolding()
   for sid, sblock in pairs(self._changed_scaffolding) do
      self:_update_scaffolding_region(sblock)
      self:_check_sblock_destroy(sblock)
   end
   self._changed_scaffolding = {}
end

function ScaffoldingManager:_find_scaffolding_for(rblock)
   -- we know the scaffolding will eventually want to cover  the entire blueprint,
   -- so use the blueprint_region to search for an sblock to merge with (especially
   -- since the actual requested scaffolding region is most likely empty at this
   -- point.  the bottom row of the project is usually trivially reachable at the
   -- time scaffolding is requested)
   local origin = rblock.origin
   local normal = rblock.normal
   local region = rblock.blueprint_region:get()

   if rblock.blueprint_clip_box then
      region = region:clipped(rblock.blueprint_clip_box)
   end

   -- look "down" for someone who can help us out.
   local zone = region:translated(origin + rblock.normal)
   zone = _physics:project_region(zone, CLIP_SOLID)

   local sblock
   radiant.terrain.get_entities_in_region(zone, function(entity)
         if not sblock then
            local fabricator, _, _ = build_util.get_fbp_for(entity)
            if fabricator then
               -- an existing fabricator overlaps the region the rblock wants
               -- to occupy.  if it's one of our scaffolding fabricators, merge
               -- with it.
               for _, sblk in pairs(self._sv.scaffolding) do
                  if sblk.fabricator == fabricator and
                     sblk.normal == rblock.normal then
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
   log:detail('bound rid:%d to existing sblock sid:%d', rblock.rid, sblock.sid)
   return true
end

function ScaffoldingManager:_create_scaffolding_for(rblock)
   checks('self', 'table')
   assert(not rblock.sblock)

   local owner = radiant.entities.get_player_id(rblock.entity)
   local origin = rblock.origin
   local normal = rblock.normal
   local sid = self:_get_next_id()

   local region = radiant.alloc_region3()
   local scaffolding = radiant.entities.create_entity('stonehearth:build:prototypes:scaffolding', { owner = owner })
   scaffolding:set_debug_text(string.format('sid:%d', sid))

   scaffolding:add_component('stonehearth:construction_data')
                  :set_normal(normal)
   scaffolding:add_component('destination')
                  :set_region(region)

   -- create a fabricator entity to build the scaffolding
   local fabricator = radiant.entities.create_entity('', { owner = owner })
   radiant.terrain.place_entity_at_exact_location(fabricator, origin)

   fabricator:set_debug_text(rblock.debug_text .. ':scaffolding:fab')
   fabricator:add_component('stonehearth:fabricator')
                              :start_project(scaffolding)

   build_util.bind_fabricator_to_blueprint(scaffolding, fabricator, 'stonehearth:fabricator')

   -- let's go go go!
   stonehearth.build:set_active(scaffolding, true)

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
   self:_trace_sblock_region(sblock)

   log:detail('created new sblock sid:%d for rblock rid:%d', rblock.rid, sblock.sid)

   return sblock
end

function ScaffoldingManager:_update_scaffolding_region(sblock)
   local sid = sblock.sid
   assert(sblock.region)

   log:detail('updating scaffolding region for sblock sid:%d', sid)

   local merged = Region3()
   for rid, rblock in pairs(sblock.regions) do
      -- the rblock region is in the coordinate space of the entity.  we're trying to build
      -- a region in world coordinate space, so move it over.
      local r = rblock.region:get():translated(rblock.origin)
      merged:add_region(r)
   end   

   log:detail('sid:%d world merged scaffolding region bounds is %s, area %d', sid, merged:get_bounds(), merged:get_area())
   merged:translate(-sblock.origin)

   sblock.region:modify(function(cursor)
         cursor:copy_region(merged)
      end)

   local new_ladder_builders = {}
   if not merged:empty() then
      for rid, rblock in pairs(sblock.regions) do
         local climb_to = self:_compute_ladder_top(rblock)
         if climb_to then
            local ladder_builder = stonehearth.build:request_ladder_to(sblock.owner,
                                                                       climb_to,
                                                                       rblock.normal, {
                                                                         pierce_roof = true
                                                                       })
            new_ladder_builders[rblock.rid] = ladder_builder
            log:detail('created ladder lbid:%d to %s for sid:%d rid:%d', ladder_builder:get_id(), climb_to, sid, rid)
         end
      end
   end
   
   for rid, ladder_builder in pairs(sblock.ladder_builders) do
      log:detail('removing old ladder builder lbid:%d from sid:%d', ladder_builder:get_id(), sid)
      ladder_builder:destroy()
   end
   sblock.ladder_builders = new_ladder_builders
   log:spam('sblock sid:%d now has %d ladder builders', sid, radiant.size(sblock.ladder_builders))
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

   -- start at the origin of the entity
   local climb_to = rblock.origin

   -- move over to the edge of the region we need to support
   local blueprint_region = rblock.blueprint_region:get()
   if rblock.blueprint_clip_box then
      blueprint_region = blueprint_region:clipped(rblock.blueprint_clip_box)
   end

   local region_min = blueprint_region:get_bounds().min
   climb_to = climb_to + region_min

   -- mover over again by twice the normal.  once is where the scaffolding
   -- will be and again so that we're oustide of it
   local normal = rblock.normal
   climb_to = climb_to + normal + normal

   -- adjust y by the height
   climb_to.y = climb_to.y + height

   return climb_to
end

return ScaffoldingManager

