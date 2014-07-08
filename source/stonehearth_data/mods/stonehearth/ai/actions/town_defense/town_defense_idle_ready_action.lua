local rng = _radiant.csg.get_default_rng()

local TownDefenseIdleReady = class()

TownDefenseIdleReady.name = 'town defense idle ready'
TownDefenseIdleReady.does = 'stonehearth:town_defense:idle'
TownDefenseIdleReady.args = {}
TownDefenseIdleReady.version = 2
TownDefenseIdleReady.priority = 1

function TownDefenseIdleReady:run(ai, entity, args)
   radiant.entities.turn_to(entity, rng:get_int(0, 359))
   
   ai:execute('stonehearth:run_effect', {
      effect = 'combat_1h_idle',
      times = rng:get_int(3, 5)
   })
end

return TownDefenseIdleReady
