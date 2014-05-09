local Entity = _radiant.om.Entity

local log = radiant.log.create_logger('combat')

local GetMeleeRange = class()

GetMeleeRange.name = 'get melee range'
GetMeleeRange.does = 'stonehearth:combat:get_melee_range'
GetMeleeRange.args = {
   target = Entity,
}
GetMeleeRange.think_output = {
   melee_range_ideal = 'number',
   melee_range_max = 'number',
}
GetMeleeRange.version = 2
GetMeleeRange.priority = 1
GetMeleeRange.weight = 1

function GetMeleeRange:start_thinking(ai, entity, args)
   local weapon = stonehearth.combat:get_melee_weapon(entity)
   if weapon == nil or not weapon:is_valid() then
      log:warning('%s has no weapon', entity)
      ai:abort('No weapon')
      return
   end
   local weapon_data = radiant.entities.get_entity_data(weapon, 'stonehearth:combat:weapon_data')
   local melee_range_ideal, melee_range_max = stonehearth.combat:get_melee_range(entity, weapon_data, args.target)
   ai:set_think_output({
      melee_range_ideal = melee_range_ideal,
      melee_range_max = melee_range_max,
   })
end

return GetMeleeRange
