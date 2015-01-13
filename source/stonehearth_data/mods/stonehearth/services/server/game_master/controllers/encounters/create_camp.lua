local Point3 = _radiant.csg.Point3
local Array2D = require 'services.server.world_generation.array_2D'
local PerturbationGrid = require 'services.server.world_generation.perturbation_grid'

local rng = _radiant.csg.get_default_rng()

local CAMP_SIZE = 3
local LARGE_SPOT_COUNT = 3
local LARGE_PIECE_RADIUS = 6

local MEDIUM_SPOT_COUNT = 3
local MEDIUM_SPOT_RADIUS = 3
local MEDIUM_SPOT_SIZE = math.floor(LARGE_PIECE_RADIUS / MEDIUM_SPOT_RADIUS)

local INFINITE = 10000000

local PIECE_SIZES = {
   large = true,
   medium = true,
}

local CreateCamp = class()

function CreateCamp:start(ctx, info)
   assert(ctx)
   assert(info)
   assert(info.pieces)

   self._sv.info = info
   self._sv.ctx = ctx

   ctx.enemy_player_id = info.player_id -- xxx: should be 'npc_enemy_id'
   self._population = stonehearth.population:get_population(ctx.enemy_player_id)

   self:_compute_camp_origin(ctx, info)

   local pieces = {
      large = {},
      medium = {},
   }

   for k, piece in pairs(info.pieces) do      
      local uri = piece.info
      piece.info = radiant.resources.load_json(uri)
      assert(piece.info.type == 'camp_piece', string.format('camp piece at "%s" does not have type == camp_piece', uri))
      assert(PIECE_SIZES[piece.info.size], string.format('camp piece at "%s" size field "%s" is invalid', uri, tostring(piece.info.size)))
      piece.info.min = piece.info.min or 0
      piece.info.max = piece.info.max or INFINITE
      table.insert(pieces[piece.info.size], piece)
   end
   local used_large_spots = {}
   self:_create_large_spots(used_large_spots, pieces.large)
   self:_create_medium_spots(used_large_spots, pieces.medium)

   -- camp created!  move onto the next encounter in the arc.
   ctx.arc:trigger_next_encounter(ctx)   
end

function CreateCamp:_create_large_spots(used_large_spots, pieces)
   local spots = self:_choose_spots(used_large_spots, CAMP_SIZE, CAMP_SIZE, LARGE_SPOT_COUNT)
   for _, spot in pairs(spots) do
      local x = spot.x * (LARGE_PIECE_RADIUS * 2) + LARGE_PIECE_RADIUS
      local z = spot.y * (LARGE_PIECE_RADIUS * 2) + LARGE_PIECE_RADIUS
      local piece = self:_choose_piece(pieces)
      if piece then
         self:_add_piece(x, z, piece.info)
      end
   end
end

function CreateCamp:_create_medium_spots(used_large_spots, pieces)
   local spots = self:_choose_spots(used_large_spots, MEDIUM_SPOT_SIZE, MEDIUM_SPOT_SIZE, LARGE_SPOT_COUNT)
   for _, spot in pairs(spots) do
      local ox = spot.x * (LARGE_PIECE_RADIUS * 2)
      local oz = spot.y * (LARGE_PIECE_RADIUS * 2)
      local subspots = self:_choose_spots({}, MEDIUM_SPOT_SIZE, MEDIUM_SPOT_SIZE, MEDIUM_SPOT_COUNT)
      for _, subspot in pairs(subspots) do
         local x = ox + subspot.x * (MEDIUM_SPOT_RADIUS * 2) + MEDIUM_SPOT_RADIUS
         local z = oz + subspot.y * (MEDIUM_SPOT_RADIUS * 2) + MEDIUM_SPOT_RADIUS
         local piece = self:_choose_piece(pieces)
         if piece then
            self:_add_piece(x, z, piece.info)
         end
      end
   end
end

function CreateCamp:_choose_piece(pieces)
   if #pieces == 0 then
      return
   end
   
   for i, piece in pairs(pieces) do
      if piece.min > 0 then
         piece.min = piece.min - 1
         piece.max = piece.max - 1
         if piece.max == 0 then
            table.remove(pieces, i)
         end
         return piece
      end
   end

   local i = rng:get_int(1, #pieces)
   local piece = pieces[i]
   piece.max = piece.max - 1
   if piece.max == 0 then
      table.remove(pieces, i)
   end
   return piece
end


function CreateCamp:_choose_spots(used_spots, w, h, count)
   local spots = {}
   local size = w * h
   count = math.min(count, size - radiant.size(used_spots))
   
   for i=1,count do
      local spot = rng:get_int(1, size)
      while used_spots[spot] do
         spot = spot + 1
         if spot > size then
            spot = 1
         end
      end
      table.insert(spots, spot)
      assert(not used_spots[spot])
      used_spots[spot] = true
   end

   for i, spot in ipairs(spots) do
      local x = math.floor(spot / w)
      local y = spot - (x * w)
      spots[i] = { x=x, y=y }
   end
   return spots
end


function CreateCamp:_compute_camp_origin(ctx, info)
   -- get the location of the town banner
   local town = stonehearth.town:get_town(ctx.player_id)
   local player_banner = town:get_banner()
   local spawn_point = radiant.entities.get_world_location(player_banner)
   local camp_radius = math.floor(CAMP_SIZE * LARGE_PIECE_RADIUS)

   -- pick a random point and go in that direction
   local info = self._sv.info
   local min = info.spawn_range.min
   local max = info.spawn_range.max
   local d = rng:get_int(min, max) + camp_radius
   local theta = rng:get_real(0, math.pi * 2)
   local offset = Point3(d * math.sin(theta), 0, d * math.cos(theta))

   -- store the origin
   ctx.enemy_location = spawn_point + offset:to_closest_int()
end

function CreateCamp:_add_piece(x, z, piece)
   local origin = self._sv.ctx.enemy_location + Point3(x, 0, z)
   local player_id = self:_get_player_id()

   -- add all the entities.
   if piece.entities then
      for name, info in pairs(piece.entities) do
         local entity = radiant.entities.create_entity(info.uri)
         local offset = Point3(info.location.x, info.location.y, info.location.z)
         radiant.terrain.place_entity(entity, origin + offset, { force_iconic = info.force_iconic })
         radiant.entities.turn_to(entity, 90 * rng:get_int(0, 3))
         radiant.entities.set_player_id(entity, player_id)
      end
   end

   -- add all the people.
   if piece.citizens then
      for name, info in pairs(piece.citizens) do
         local citizen = self._population:create_new_citizen()
         if info.job then
            citizen:add_component('stonehearth:job')
                        :promote_to(info.job)
         end
         local offset = Point3(info.location.x, info.location.y, info.location.z)
         radiant.terrain.place_entity(citizen, origin + offset)
      end
   end
end

function CreateCamp:_get_player_id()
   return self._sv.info.player_id
end

return CreateCamp
