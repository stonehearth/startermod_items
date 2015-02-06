local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3

local rng = _radiant.csg.get_default_rng()
local log = radiant.log.create_logger('game_master')

local CreateCamp = class()

function CreateCamp:start(ctx, info)
   assert(ctx)
   assert(info)
   assert(info.pieces)

   self._sv.info = info
   self._sv.ctx = ctx

   ctx.enemy_player_id = info.player_id -- xxx: should be 'npc_enemy_id'
   self:_search_for_camp_location()
end

function CreateCamp:stop()
   -- nothing to do
end

function CreateCamp:_create_camp()
   local ctx = self._sv.ctx
   local info = self._sv.info

   assert(ctx.enemy_player_id)
   assert(ctx.enemy_location)

   self._population = stonehearth.population:get_population(ctx.enemy_player_id)

   -- carve a hole in the ground for the camp to sit in
   local cube = Cube3(ctx.enemy_location - Point3(30, 1, 30), ctx.enemy_location + Point3(30 + 1, 0, 30 + 1))
   local cube2 = cube:translated(Point3.unit_y)

   local entities = radiant.terrain.get_entities_in_cube(cube2)
   
   for id, entity in pairs(entities) do
      radiant.entities.destroy_entity(entity)
   end
   
   radiant.terrain.subtract_cube(cube)

   

   if info.npc_boss_entity_type then
      local npc_boss_entity = self._population:create_new_citizen(info.npc_boss_entity_type)
      radiant.terrain.place_entity(npc_boss_entity, ctx.enemy_location)
      ctx.npc_boss_entity = npc_boss_entity
   end

   for k, piece in pairs(info.pieces) do
      local uri = piece.info
      piece.info = radiant.resources.load_json(uri)
      assert(piece.info.type == 'camp_piece', string.format('camp piece at "%s" does not have type == camp_piece', uri))
      --table.insert(pieces[piece.info.size], piece)
      self:_add_piece(piece)
   end

   -- if there's a script associated with the mod, give it a chance to customize the camp
   if info.script then
      local script = radiant.create_controller(info.script, ctx)
      self._sv.script = script
      script:start(ctx)
   end
   
   -- camp created!  move onto the next encounter in the arc.
   ctx.arc:trigger_next_encounter(ctx)   
end

function CreateCamp:_create_piece(piece)
   local x = piece.position.x
   local z = piece.position.y
   local rot = piece.rotation

   self:_add_piece(x, z, rot, piece.info)
end

function CreateCamp:_destroy_find_location_timer()
   if self._find_location_timer then
      self._find_location_timer:destroy()
      self._find_location_timer = nil
   end
end

function CreateCamp:_start_find_location_timer()
   self:_destroy_find_location_timer()
   self._find_location_timer = radiant.set_realtime_timer(15000, function()
         self:_search_for_camp_location()
      end)
end

function CreateCamp:_search_for_camp_location()
   self:_destroy_find_location_timer()

   local ctx = self._sv.ctx
   local info = self._sv.info

   -- choose a distance and start looking for a place to put the camp
   local player_banner = stonehearth.town:get_town(ctx.player_id)
                                             :get_banner()
   local min = info.spawn_range.min
   local max = info.spawn_range.max
   local d = rng:get_int(min, max)

   -- we use a goblin for pathfinding.  sigh.  ideally we should be able to pass in a mob type
   -- (e.g. HUMANOID) so we avoid creating this guy
   if not self._sv.find_camp_location_entity then   
      self._sv.find_camp_location_entity = radiant.entities.create_entity('stonehearth:monsters:goblins:goblin')
   end

   local success = function(location)
      log:info('found camp location %s', location)
      self:_finalize_camp_location(location)
   end

   local fail = function()
      log:info('failed to create camp.  trying again')
      self:_start_find_location_timer()
   end
   log:info('looking for camp location %d units away', d)

   local attempted = stonehearth.spawn_region_finder:find_point_outside_civ_perimeter_for_entity_astar(self._sv.find_camp_location_entity, player_banner, d, 1, d * 100, success, fail) 
   if not attempted then
      self:_start_find_location_timer()
   end
end

function CreateCamp:_finalize_camp_location(location)
   self:_destroy_find_location_timer()
   radiant.entities.destroy_entity(self._sv.find_camp_location_entity)

   self._sv.find_camp_location_entity = nil
   self._sv.ctx.enemy_location = location
   self.__saved_variables:mark_changed()

   self:_create_camp()
end

function CreateCamp:_add_piece(piece)
   local x = piece.position.x
   local z = piece.position.y
   local rot = piece.rotation

   local origin = self._sv.ctx.enemy_location + Point3(x, 0, z)
   local player_id = self:_get_player_id()

   -- add all the entities.
   if piece.info.entities then
      for name, info in pairs(piece.info.entities) do
         local entity = radiant.entities.create_entity(info.uri)
         local offset = Point3(info.location.x, info.location.y, info.location.z)
         radiant.terrain.place_entity(entity, origin + offset, { force_iconic = info.force_iconic })

         if rot then
            radiant.entities.turn_to(entity, rot)
         end

         radiant.entities.set_player_id(entity, player_id)
      end
   end

   -- add all the people.
   if piece.info.citizens then
      for name, info in pairs(piece.info.citizens) do
         local citizen = self._population:create_new_citizen()
         if info.job then
            citizen:add_component('stonehearth:job')
                        :promote_to(info.job)
         end
         local offset = Point3(info.location.x, info.location.y, info.location.z)
         radiant.terrain.place_entity(citizen, origin + offset)
      end
   end

   -- if there's a script associated with the mod, give it a chance to customize the camp
   if piece.info.script then
      local script = radiant.create_controller(piece.info.script, piece)
      script:start(self._sv.ctx)
   end

end

function CreateCamp:_get_player_id()
   return self._sv.info.player_id
end

return CreateCamp
