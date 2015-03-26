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

function ScaffoldingManager:request_scaffolding_for(requestor, blueprint_rgn, project_rgn, normal)
   checks('self', 'Entity', 'Region3Boxed', 'Region3Boxed', '?Point3')

   local size = blueprint_rgn:get():get_bounds():get_size()
   local builder_type
   if size.x == 1 or size.z == 1 then
      builder_type = 'stonehearth:build:scaffolding_builder_1d'
   else
      builder_type = 'stonehearth:build:scaffolding_builder_2d'
   end
   return self:_create_builder(builder_type, requestor, blueprint_rgn, project_rgn, normal)
end

function ScaffoldingManager:_get_next_id()
   local id = self._sv.next_id 
   self._sv.next_id = self._sv.next_id + 1
   self.__saved_variables:mark_changed()
   return id
end

function ScaffoldingManager:_create_builder(builder_type, requestor, blueprint_rgn, project_rgn, normal)
   checks('self', 'string', 'Entity', 'Region3Boxed', 'Region3Boxed', '?Point3')
   
   local rid = self:_get_next_id()
   local builder = radiant.create_controller(builder_type, self, rid, requestor, blueprint_rgn, project_rgn, normal)
   
   self._sv.builders[rid] = builder
   self.__saved_variables:mark_changed()
   
   return builder
end

function ScaffoldingManager:_add_region(rid, origin, region, normal)
   checks('self', 'number', 'Point3', 'Region3Boxed', 'Point3')

   assert(not self._sv.regions[rid])

   local rblock = {
      rid    = rid,
      origin = origin,
      region = region,
      normal = normal,
      owner  = 'player_1',       -- xxx
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
   if rblock.sblock then
      self._changed_scaffolding[rblock.sblock] = true
   else
      self._new_regions[rblock] = true
   end
end

function ScaffoldingManager:_process_changes()
   self._marked_changed = false
   self:_process_new_regions()
   self:_process_changed_scaffolding()
end

function ScaffoldingManager:_process_new_regions()
   for rblock, _ in pairs(self._new_regions) do
      assert(not rblock.sblock)
      
      rblock.sblock = self:_create_new_scaffolding(rblock)
      self._changed_scaffolding[rblock.sblock] = true
   end
   self._new_regions = {}
end

function ScaffoldingManager:_process_changed_scaffolding()
   for sblock, _ in pairs(self._changed_scaffolding) do
      self:_update_scaffolding_region(sblock)
   end
   self._changed_scaffolding = {}
end

function ScaffoldingManager:_create_new_scaffolding(rblock)
   checks('self', 'table')

   local owner = rblock.owner
   local origin = rblock.origin
   local normal = rblock.normal

   local region = radiant.alloc_region3()
   local scaffolding = radiant.entities.create_entity('stonehearth:scaffolding', { owner = owner })

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

   -- wire up the back pointer so we can find the fab entity from the blueprint
   scaffolding:add_component('stonehearth:construction_progress')   
                 :set_fabricator_entity(fabricator, 'stonehearth:fabricator')

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
      regions     = { [rblock] = true }
   }
   self._sv.scaffolding[sid] = sblock
   return sblock
end

function ScaffoldingManager:_update_scaffolding_region(sblock)
   local merged = Region3()
   for rblock, _ in pairs(sblock.regions) do
      local r = rblock.region:get():translated(rblock.origin)
      merged:add_region(r)
   end   
   merged:translate(-sblock.origin)
   
   sblock.region:modify(function(cursor)
         cursor:copy_region(merged)
      end)

   local climb_to = self:_compute_ladder_top(sblock)
   local old_ladder_builder = sblock.ladder_builder
   if climb_to then
      sblock.ladder_builder = stonehearth.build:request_ladder_to(sblock.owner,
                                              climb_to,
                                              sblock.normal)
   end
   if old_ladder_builder then
      old_ladder_builder:destroy()
   end
end

function ScaffoldingManager:_compute_ladder_top(sblock)
   local normal = sblock.normal
   local ladder_stencil = Region3(Cube3(Point3(0, -INFINITE, 0) + normal,
                                        Point3(1,  INFINITE, 1) + normal))
   local ladder = sblock.region:get():intersect_region(ladder_stencil)
   local height = ladder:get_bounds().max.y - 1
   if height == 0 then
      return
   end

   local normal = sblock.normal
   local climb_to = sblock.origin + normal + normal
   climb_to.y = climb_to.y + height

   return climb_to
end

return ScaffoldingManager
