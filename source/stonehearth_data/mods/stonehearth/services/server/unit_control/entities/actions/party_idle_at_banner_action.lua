local Point3 = _radiant.csg.Point3

local PartyIdleAtBannerAction = class()

PartyIdleAtBannerAction.name = 'party idle at banner'
PartyIdleAtBannerAction.does = 'stonehearth:party:move_to_banner'
PartyIdleAtBannerAction.args = {
   party    = 'table',     -- the party
   location = Point3,      -- world location of formation position
}
PartyIdleAtBannerAction.version = 2
PartyIdleAtBannerAction.priority = 1


function PartyIdleAtBannerAction:run(ai, entity, args)
   while true do
      ai:execute('stonehearth:idle', { hold_position = true })      
   end
end

return PartyIdleAtBannerAction
