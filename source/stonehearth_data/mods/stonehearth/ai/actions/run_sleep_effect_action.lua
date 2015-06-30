local RunSleepEffect = class()
RunSleepEffect.name = 'sleeping'
RunSleepEffect.does = 'stonehearth:run_sleep_effect'
RunSleepEffect.args = {
   duration_string = 'string'
}
RunSleepEffect.version = 2
RunSleepEffect.priority = 1

function RunSleepEffect:run(ai, entity, args)
   if not radiant.entities.has_buff(entity, 'stonehearth:buffs:sleeping') then
      radiant.entities.add_buff(entity, 'stonehearth:buffs:sleeping')
   end

   ai:execute('stonehearth:run_effect_timed', { effect = 'sleep', duration = args.duration_string })
end

function RunSleepEffect:stop(ai, entity, args)
   radiant.entities.remove_buff(entity, 'stonehearth:buffs:sleeping');
end

return RunSleepEffect
