local rng = _radiant.csg.get_default_rng()

local TownDefenseIdleReady = class()

TownDefenseIdleReady.name = 'town defense idle ready'
TownDefenseIdleReady.does = 'stonehearth:town_defense:idle'
TownDefenseIdleReady.args = {}
TownDefenseIdleReady.version = 2
TownDefenseIdleReady.priority = 1

function TownDefenseIdleReady:run(ai, entity, args)
   ai:execute('stonehearth:run_effect', {
      effect = 'combat_1h_idle',
      times = rng:get_int(3, 5)
   })

   ai:execute('stonehearth:wander', {
      radius_min = 2,
      radius = 5
   })
end

return TownDefenseIdleReady
