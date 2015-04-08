local game_master_lib = require 'lib.game_master.game_master_lib'

local Point3 = _radiant.csg.Point3
local Cube3 = _radiant.csg.Cube3

local Point2 = _radiant.csg.Point2
local Rect2 = _radiant.csg.Rect2
local Region2 = _radiant.csg.Region2

local log = radiant.log.create_logger('game_master')

local VISION_PADDING = Point2(16, 16)
local CreateCamp = class()

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
   self._sv.searcher = radiant.create_controller('stonehearth:game_master:util:choose_location_outside_town',
                                                 ctx.player_id, min, max,
                                                 radiant.bind_callback(self, '_create_camp'))
end

function CreateCamp:stop()
   -- nothing to do
   if self._sv.searcher then
      self._sv.searcher:destroy()
      self._sv.searcher = nil      
   end
end

function CreateCamp:_create_camp(location)
   local ctx = self._sv.ctx
   local info = self._sv._info

   assert(info.npc_player_id)
   ctx.enemy_location = location

   self._population = stonehearth.population:get_population(info.npc_player_id)

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
      local members = game_master_lib.create_citizens(self._population, info.boss, ctx.enemy_location)

      for k, boss in pairs(members) do
         ctx.npc_boss_entity = boss
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
end

function CreateCamp:_add_piece(piece, visible_rgn)
   local x = piece.position.x
   local z = piece.position.y
   local rot = piece.rotation

   local ctx = self._sv.ctx
   local info = self._sv._info

   local player_id = info.npc_player_id
   local origin = ctx.enemy_location + Point3(x, 0, z)
   
   -- add all the entities.
   if piece.info.entities then
      for name, info in pairs(piece.info.entities) do
         local entity = radiant.entities.create_entity(info.uri, { owner = player_id })
         local offset = Point3(info.location.x, info.location.y, info.location.z)
         radiant.terrain.place_entity(entity, origin + offset, { force_iconic = info.force_iconic })
         if rot then
            radiant.entities.turn_to(entity, rot)
         end
         self:_add_entity_to_visible_rgn(entity, visible_rgn)

         --TODO: add this entity to the ctx
         ctx[name] = entity
      end
   end

   -- add all the people.
   if piece.info.citizens then
      for name, info in pairs(piece.info.citizens) do
         local members = game_master_lib.create_citizens(self._population, info, origin)
         for id, member in pairs(members) do
            self:_add_entity_to_visible_rgn(member, visible_rgn)
            
            --TODO: Stephanie, add all these people to the context. here's the old code.
            --ctx[name] = citizen
         end        
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
