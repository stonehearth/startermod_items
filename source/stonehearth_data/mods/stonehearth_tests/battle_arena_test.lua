local MicroWorld = require 'lib.micro_world'
local Point3 = _radiant.csg.Point3
local game_master_lib = require 'stonehearth.lib.game_master.game_master_lib'

local BattleArenaTest = class(MicroWorld)


function BattleArenaTest:__init()
   self[MicroWorld]:__init(64)
   self:create_world()

   self:place_item('stonehearth:large_oak_tree', -25, -25)

   local battle_data = radiant.resources.load_json('/stonehearth_tests/data/battle_arena/workers_vs_goblins.json')
   self:load_teams(battle_data)

   self:at(100, function()
         --tree:get_component('stonehearth:commands'):do_command('chop', player_id)
      end)
end

function BattleArenaTest:load_teams(battle_data)
   for _, team in pairs(battle_data) do
      self:load_team(team)
   end
end

function BattleArenaTest:load_team(team)
   local citizen_count = 0
   local population = stonehearth.population:get_population(team.player_id)
   
   for key, citizen_spec in pairs(team.members) do
      if not key:starts_with('_')  then
         local count = citizen_spec.count or 1
         for i = 1,count do
            local location = Point3(team.location.x + citizen_count * 4, team.location.y, team.location.z)
            local citizen = game_master_lib.create_citizen(population, citizen_spec, location)

            if team.player_id == 'player_1' then
               radiant.entities.turn_to(citizen, 180)
            end

            citizen_count = citizen_count + 1
         end
      end
   end
end


return BattleArenaTest

