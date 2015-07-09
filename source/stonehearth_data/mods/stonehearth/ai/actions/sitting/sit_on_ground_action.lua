local SitOnGround = class()
SitOnGround.name = 'sit on ground'
SitOnGround.does = 'stonehearth:sit_on_ground'
SitOnGround.args = { }
SitOnGround.version = 2
SitOnGround.priority = 1

function SitOnGround:run(ai, entity)
   ai:execute('stonehearth:run_effect', { effect = 'sit_on_ground' })
   radiant.entities.set_posture(entity, 'stonehearth:sitting')
end

function SitOnGround:stop(ai, entity)
   radiant.entities.unset_posture(entity, 'stonehearth:sitting')
end

return SitOnGround
