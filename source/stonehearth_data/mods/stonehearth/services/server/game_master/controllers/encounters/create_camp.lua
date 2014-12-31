local Point3 = _radiant.csg.Point3
local rng = _radiant.csg.get_default_rng()

local CreateCamp = class()

function CreateCamp:start(ctx, info)
   self._sv.info = info

   self:_compute_camp_origin(ctx)

   local population = stonehearth.population:get_population(info.player_id)
   if info.optional_pieces then
      for _, uri in pairs(info.optional_pieces.pieces) do
         self:_add_piece(population, uri)
      end
   end

   -- camp created!  move onto the next encounter in the arc.
   ctx.arc:trigger_next_encounter(ctx)
end

function CreateCamp:_compute_camp_origin(ctx)
   -- get the location of the town banner
   local town = stonehearth.town:get_town(ctx.player_id)
   local player_banner = town:get_banner()
   local spawn_point = radiant.entities.get_world_location(player_banner)

   -- pick a random point and go in that direction
   local info = self._sv.info
   local min = info.spawn_range.min
   local max = info.spawn_range.max
   local d = rng:get_int(min, max)
   local theta = rng:get_real(0, math.pi * 2)
   local offset = Point3(d * math.sin(theta), 0, d * math.cos(theta))

   -- store the origin
   self._sv.origin = spawn_point + offset:to_closest_int()
end

function CreateCamp:_add_piece(population, uri)
   local piece = radiant.resources.load_json(uri)
   local origin = self._sv.origin
   local player_id = self:_get_player_id()

   -- add all the entities.
   for name, info in pairs(piece.entities) do
      local entity = radiant.entities.create_entity(info.uri)
      local offset = Point3(info.location.x, info.location.y, info.location.z)
      radiant.terrain.place_entity(entity, origin + offset, { force_iconic = info.force_iconic })
      radiant.entities.set_player_id(entity, player_id)
   end
   
   -- add all the people.
   for name, info in pairs(piece.citizens) do
      local citizen = population:create_new_citizen()
      if info.job then
         citizen:add_component('stonehearth:job')
                     :promote_to(info.job)
      end
      local offset = Point3(info.location.x, info.location.y, info.location.z)
      radiant.terrain.place_entity(citizen, origin + offset)
   end
end

function CreateCamp:_get_player_id()
   return self._sv.info.player_id
end


return CreateCamp
