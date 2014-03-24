local GameMasterService = class()
local WolfScenario = require 'services.game_master.scenarios.nightly_wolf_attack'

function GameMasterService:__init()
   self._scenarions = {}
end

function GameMasterService:initialize()
end

function GameMasterService:start()
   -- xxx, hardcoded. Make data driven
   local scenario = WolfScenario()
   table.insert(self._scenarions, scenario)
end

return GameMasterService

