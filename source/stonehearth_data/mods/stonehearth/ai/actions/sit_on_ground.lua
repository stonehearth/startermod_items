local event_service = stonehearth.events

local SitOnGround = class()
SitOnGround.name = 'sit on ground'
SitOnGround.does = 'stonehearth:sit_on_ground'
SitOnGround.args = { }
SitOnGround.version = 2
SitOnGround.priority = 1

function SitOnGround:run(ai, entity)
   radiant.entities.set_posture(entity, 'sitting')
   ai:execute('stonehearth:run_effect', 'sit_on_ground')
end

function SitOnGround:stop(ai, entity)
   radiant.entities.unset_posture(entity, 'sitting')
end

return SitOnGround
