local game_master_lib = require 'lib.game_master.game_master_lib'

local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3

local rng = _radiant.csg.get_default_rng()
local log = radiant.log.create_logger('game_master')

local CreateCamp = class()

function CreateCamp:start(ctx, info)
   assert(ctx)
   assert(info)
   assert(info.pieces)

   self._sv._info = info
   self._sv.ctx = ctx

   ctx.enemy_player_id = info.player_id -- xxx: should be 'npc_enemy_id'
   self:_search_for_camp_location()
end

function CreateCamp:stop()
   -- nothing to do
end

function CreateCamp:_create_camp()
   local ctx = self._sv.ctx
   local info = self._sv._info

   assert(ctx.enemy_player_id)
   assert(ctx.enemy_location)

   self._population = stonehearth.population:get_population(ctx.enemy_player_id)

   -- carve a hole in the ground for the camp to sit in
   local size = 20
   local cube = Cube3(ctx.enemy_location - Point3(size, 1, size), ctx.enemy_location + Point3(size + 1, 0, size + 1))
   local cube2 = cube:translated(Point3.unit_y)

   -- nuke all the entities around the camp
   local entities = radiant.terrain.get_entities_in_cube(cube2)
   for id, entity in pairs(entities) do
      if id ~= 1 then
         radiant.entities.destroy_entity(entity)
      end
   end
   
   -- carve out the grass around the camp
   radiant.terrain.subtract_cube(cube)

   -- create the boss entity
   if info.boss then
      ctx.npc_boss_entity = game_master_lib.create_citizen(self._population, info.boss, ctx.enemy_location)
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
   local info = self._sv._info

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
   
   -- add all the entities.
   if piece.info.entities then
      for name, info in pairs(piece.info.entities) do
         local entity = radiant.entities.create_entity(info.uri, { owner = ctx.enemy_player_id })
         local offset = Point3(info.location.x, info.location.y, info.location.z)
         radiant.terrain.place_entity(entity, origin + offset, { force_iconic = info.force_iconic })

         if rot then
            radiant.entities.turn_to(entity, rot)
         end
      end
   end

   -- add all the people.
   if piece.info.citizens then
      for name, info in pairs(piece.info.citizens) do
         game_master_lib.create_citizen(self._population, info, origin)
      end
   end

   -- if there's a script associated with the mod, give it a chance to customize the camp
   if piece.info.script then
      local script = radiant.create_controller(piece.info.script, piece)
      script:start(self._sv.ctx)
   end

end

return CreateCamp
