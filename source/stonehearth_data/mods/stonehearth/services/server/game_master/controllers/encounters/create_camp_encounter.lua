local build_util = require 'lib.build_util'
local game_master_lib = require 'lib.game_master.game_master_lib'

local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3

local Point2 = _radiant.csg.Point2
local Rect2 = _radiant.csg.Rect2
local Region2 = _radiant.csg.Region2
local Region3 = _radiant.csg.Region3

local log = radiant.log.create_logger('game_master')

local VISION_PADDING = Point2(16, 16)
local CreateCamp = class()

function CreateCamp:activate()
   self._log = radiant.log.create_logger('game_master.create_camp')
                              :set_prefix('create camp')
end

function CreateCamp:start(ctx, info)
   assert(ctx)
   assert(info)
   assert(info.pieces)
   assert(info.npc_player_id)
   assert(info.spawn_range)
   assert(info.spawn_range.min)
   assert(info.spawn_range.max)

   local min = info.spawn_range.min
   local max = info.spawn_range.max

   ctx.npc_player_id = info.npc_player_id
   self._sv.ctx = ctx
   self._sv._info = info

   --create shape of the camp, based on params, as a region
   --create 1x1 cube
   local cube = Cube3(Point3.zero, Point3.one)
   --inflate by size of radius
   cube = cube:inflated(Point3(info.radius, 0, info.radius))
   self._sv.camp_region = Region3(cube)

   --Create a controller to find the location, pass params
   self._sv.searcher = radiant.create_controller('stonehearth:game_master:util:choose_location_outside_town',
                                                 ctx.player_id, min, max,
                                                 radiant.bind(self, '_choose_camp_cb'), 
                                                 self._sv.camp_region)
end

function CreateCamp:stop()
   -- nothing to do
   if self._sv.searcher then
      self._sv.searcher:destroy()
      self._sv.searcher = nil      
   end
end

function CreateCamp:_choose_camp_cb(op, location, camp_region)
   checks('self', 'string', 'Point3', 'Region3')

   if op == 'check_location' then
      return self:_test_camp(location, camp_region)
   elseif op == 'set_location' then
      return self:_create_camp(location, camp_region)
   else
      radiant.error('unknown op "%s" in choose camp callback', op)
   end
end

function CreateCamp:_classify_entity_in_camp_region(id, entity)
   -- the ground is fine
   if id == 1 then
      return 'ok'
   end

   -- resources and items should be destroyed
   if entity:get_component('item') or
      entity:get_component('stonehearth:resource_node') or
      entity:get_component('stonehearth:renewable_resource_node') then
      return 'destroy'
   end

   -- no water and not in a mine.
   if entity:get_component('stonehearth:water') or
      entity:get_component('stonehearth:waterfall') or      
      entity:get_component('stonehearth:mining_zone') then
      return 'invalid'
   end

   -- no buildings
   if build_util.get_building_for(entity) then
      return 'invalid'
   end

   -- or anything else solid
   if radiant.entities.is_solid_entity(entity) then
      return 'invalid'
   end

   -- everything else is ok!
   return 'ok'
end

function CreateCamp:_test_camp(location, camp_region)
   -- check everything in the region and make sure we're ok to be here.
   -- we look 1 down, too, since we may end up sinking the camp.
   local query_region = Region3()
   query_region:copy_region(camp_region)
   query_region:add_region(camp_region:translated(-Point3.unit_y))

   local entities = radiant.terrain.get_entities_in_region(query_region)
   for id, entity in pairs(entities) do
      if self:_classify_entity_in_camp_region(id, entity) == 'invalid' then
         self._log:detail('%s inside potential camp location %s.  rejecting.', entity, location)
         return false
      end
   end
   return true
end

function CreateCamp:_create_camp(location, camp_region)
   checks('self', 'Point3', 'Region3')

   local ctx = self._sv.ctx
   local info = self._sv._info

   assert(info.npc_player_id)
   ctx.enemy_location = location

   self._population = stonehearth.population:get_population(info.npc_player_id)

   -- change the amenity between the owner of the camp and the player we're creating
   -- the encounter for.
   if info.amenity_with_player then
      stonehearth.player:set_amenity(info.npc_player_id, ctx.player_id, info.amenity_with_player)
   end

   -- nuke all the entities around the camp
   local entities = radiant.terrain.get_entities_in_region(camp_region)
   for id, entity in pairs(entities) do
      if self:_classify_entity_in_camp_region(id, entity) == 'destroy' then
         radiant.entities.destroy_entity(entity)
      end
   end
   
   -- carve out the grass around the camp, but only if there's grass there (and if the encounter
   -- hasn't specified to keep the grass), The check is largely to
   -- prevent digging a hole in a hole, which causes pathfinding problems (e.g. if we pick the same
   -- camp location twice.)
   local kind = radiant.terrain.get_block_kind_at(location - Point3.unit_y)
   if kind == 'grass' and not self._sv._info.keep_grass then
      local terrain_region = camp_region:translated(-Point3.unit_y)
      radiant.terrain.subtract_region(terrain_region)
      location.y = location.y - Point3.unit_y.y
   end

   -- create the boss entity
   if info.boss then
      local members = game_master_lib.create_citizens(self._population,
                                                      info.boss,
                                                      ctx.enemy_location,
                                                      ctx)
      local ctx_entity_registration_path = self._sv._info.ctx_entity_registration_path
      if ctx_entity_registration_path then
         --This is a bit weird. There's just one boss, really. Last boss gets it
         --TODO: if this becomes a problem later, we can fix it them
         local boss_entity = members[1]
         if boss_entity then
            game_master_lib.register_entities(ctx, ctx_entity_registration_path, { boss = boss_entity })
         end
      end
   end

   local visible_rgn = Region2()
   for k, piece in pairs(info.pieces) do
      local uri = piece.info
      piece.info = radiant.resources.load_json(uri)
      assert(piece.info.type == 'camp_piece', string.format('camp piece at "%s" does not have type == camp_piece', uri))
      --table.insert(pieces[piece.info.size], piece)
      self:_add_piece(piece, visible_rgn)
   end

   -- add to the visible region for that player
   stonehearth.terrain:get_explored_region(ctx.player_id)
                           :modify(function(cursor)
                                 cursor:add_region(visible_rgn)
                              end)


   -- if there's a script associated with the mod, give it a chance to customize the camp
   if info.script then
      local script = radiant.create_controller(info.script, ctx)
      self._sv.script = script
      script:start(ctx, info)
   end
   
   -- camp created!  move onto the next encounter in the arc.
   ctx.arc:trigger_next_encounter(ctx)   

   -- Debug event! If someone is listening when the camp is created (autotests) let them know
   radiant.events.trigger_async(ctx.encounter_name, 'stonehearth:create_camp_complete', {}) 
end

function CreateCamp:_add_piece(piece, visible_rgn)
   local x = piece.position.x
   local z = piece.position.y
   local rot = piece.rotation

   local ctx = self._sv.ctx
   local info = self._sv._info

   local player_id = info.npc_player_id
   local origin = ctx.enemy_location + Point3(x, 0, z)
   local ctx_entity_registration_path = self._sv._info.ctx_entity_registration_path
   
   -- add all the entities.
   local entities = {}
   if piece.info.entities then
      for name, info in pairs(piece.info.entities) do
         local entity = game_master_lib.create_entity(info, player_id)
         local offset = Point3(info.location.x, info.location.y, info.location.z)
         radiant.terrain.place_entity(entity, origin + offset, { force_iconic = info.force_iconic })
         if rot then
            radiant.entities.turn_to(entity, rot)
         end
         self:_add_entity_to_visible_rgn(entity, visible_rgn)

         --Add this entity to the ctx
         entities[name] = entity
      end
   end
   if ctx_entity_registration_path then
      game_master_lib.register_entities(ctx, ctx_entity_registration_path .. '.entities', entities)
   end


   --Go through and create all the citizens. Put them in a table so we can neatly 
   --add them all to the context, all at once
   local citizens_by_type = {}
   if piece.info.citizens then
      for type, info in pairs(piece.info.citizens) do 
         local citizens = game_master_lib.create_citizens(self._population, info, origin, ctx)
         citizens_by_type[type] = citizens
         for i, citizen in ipairs(citizens) do
            self:_add_entity_to_visible_rgn(citizen, visible_rgn)
         end
      end
      if ctx_entity_registration_path then
         --This fn adds everyone to the ctx in a mechanism like ctx.enc_name.citizens.type
         game_master_lib.register_entities(ctx, ctx_entity_registration_path .. '.citizens', citizens_by_type)
      end
   end

   -- if there's a script associated with the mod, give it a chance to customize the camp
   if piece.info.script then
      local script = radiant.create_controller(piece.info.script, piece)
      script:start(self._sv.ctx, piece.info.script_info)
   end

end

function CreateCamp:_add_entity_to_visible_rgn(entity, visble_rgn)
   local location = radiant.entities.get_world_grid_location(entity)
   local pt = Point2(location.x, location.z)
   visble_rgn:add_cube(Rect2(pt - VISION_PADDING, pt + VISION_PADDING))

   local dst = entity:get_component('destination')
   if dst then
      self:_add_region_to_visble_rgn(entity, dst:get_region(), visble_rgn)
   end
   local rcs = entity:get_component('region_collision_shape')
   if rcs then
      self:_add_region_to_visble_rgn(entity, rcs:get_region(), visble_rgn)
   end
   local stockpile = entity:get_component('stockpile')
   if stockpile then
      local size = stockpile:get_size()
      visble_rgn:add_cube(Rect2(size - VISION_PADDING, size + VISION_PADDING))
   end

end

function CreateCamp:_add_region_to_visble_rgn(entity, rgn, visble_rgn)
   if not rgn then
      return
   end

   local world_rgn = radiant.entities.local_to_world(rgn:get(), entity)
   for cube in world_rgn:each_cube() do
      local min = Point2(cube.min.x, cube.min.z) - VISION_PADDING
      local max = Point2(cube.max.x, cube.max.z) + VISION_PADDING
      visble_rgn:add_cube(Rect2(min, max))
   end
end

return CreateCamp
