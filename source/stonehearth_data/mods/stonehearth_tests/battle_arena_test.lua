local MicroWorld = require 'lib.micro_world'
local Point3 = _radiant.csg.Point3
local game_master_lib = require 'stonehearth.lib.game_master.game_master_lib'

local BattleArenaTest = class(MicroWorld)


function BattleArenaTest:__init()
   self[MicroWorld]:__init(64)
   self:create_world()

   self:place_item('stonehearth:large_oak_tree', -25, -25)

   local battle_data = radiant.resources.load_json('/stonehearth_tests/data/battle_arena/workers_vs_undead.json')
   self:load_teams(battle_data)


   self:at(100, function()
         stonehearth.calendar:set_time_unit_test_only({ hour = stonehearth.constants.sleep.BEDTIME_START - 1, minute = 58 })
      end)
end

function BattleArenaTest:load_teams(battle_data)
   for _, team in pairs(battle_data) do
      self:load_team(team)
   end
end

function BattleArenaTest:load_team(info)
   local citizen_count = 0
   local population = stonehearth.population:get_population(info.player_id)
   local offset = Point3(info.offset.x, info.offset.y, info.offset.z)

   for key, citizen_spec in pairs(info.members) do
      if not key:starts_with('_')  then

         for name, info in pairs(info.members) do
            local citizens = game_master_lib.create_citizens(population, info, offset)
            if info.player_id == 'player_1' then
               for i, citizen in ipairs(citizens) do
                  radiant.entities.turn_to(citizen, 180)
               end
            end
         end
      end
   end
end


return BattleArenaTest

