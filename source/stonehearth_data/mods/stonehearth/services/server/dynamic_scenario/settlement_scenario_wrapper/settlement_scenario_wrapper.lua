local Point3 = _radiant.csg.Point3

local rng = _radiant.csg.get_default_rng()

local SettlementScenario = class()

function SettlementScenario:initialize(data)
   self._sv._running = false
   self._sv._data = data
end

function SettlementScenario:destroy()
   -- radiant.destroy_controller(self._sv._scenario)
end

function SettlementScenario:restore()
end

function SettlementScenario:can_spawn()
   local player_town = stonehearth.town:get_town('player_1')
   if not player_town then
      return false
   end
   local banner = player_town:get_banner()
   if not banner then
      return false
   end
   self.__saved_variables:modify(function(o)
         o.player_banner = banner
      end)

   return true
end

function SettlementScenario:is_running()
   return self._sv._running
end

function SettlementScenario:start()
   assert(not self._sv._running)
   assert(self._sv.player_banner)

   self._sv._running = true
   self:_spawn_settlement()
end

function SettlementScenario:_on_finished()
   self._sv._running = false
   self.__saved_variables:mark_changed()
end

function SettlementScenario:_spawn_settlement()
   local spawn_point = radiant.entities.get_world_location(self._sv.player_banner)

   -- pick a random point and go in that direction
   local data = self._sv._data
   local min = data.settlement_spawn_range.min
   local max = data.settlement_spawn_range.max
   local d = rng:get_int(min, max)
   local theta = rng:get_real(0, math.pi * 2)
   local offset = Point3(d * math.sin(theta), 0, d * math.cos(theta))

   self._sv.population = stonehearth.population:get_population(data.settlement_player_id)


   self._sv.origin = spawn_point + offset:to_closest_int()
   for _, uri in pairs(data.settlement_pieces) do
      self:_add_settlement_piece(uri)
   end
end

function SettlementScenario:_add_settlement_piece(uri)
   local piece = radiant.resources.load_json(uri)
   local origin = self._sv.origin
   for name, info in pairs(piece.entities) do
      local entity = radiant.entities.create_entity(info.uri)
      local offset = Point3(info.location.x, info.location.y, info.location.z)
      radiant.terrain.place_entity(entity, origin + offset, { force_iconic = info.force_iconic })
   end
   for name, info in pairs(piece.citizens) do
      local citizen = self._sv.population:create_new_citizen()
      if info.job then
         citizen:add_component('stonehearth:job')
                     :promote_to(info.job)
      end
      local offset = Point3(info.location.x, info.location.y, info.location.z)
      radiant.terrain.place_entity(citizen, origin + offset)
   end
end


return SettlementScenario
