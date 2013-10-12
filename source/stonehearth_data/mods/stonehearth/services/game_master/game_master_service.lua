local GameMasterService = class()
local WolfScenario = require 'services.game_master.scenarios.nightly_wolf_attack'

function GameMasterService:__init()
   -- this is total bs...sketching in code I guess? 
   self._scenarions = {}
   local scenario = WolfScenario()
   table.insert(self._scenarions, scenario)
end

return GameMasterService()
