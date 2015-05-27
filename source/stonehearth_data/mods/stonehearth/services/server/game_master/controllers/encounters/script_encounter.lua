
--Simple encounter does nothing but run the associated script (which is created as a controller)

local ScriptEncounter = class()

function ScriptEncounter:activate()
   self._log = radiant.log.create_logger('game_master.encounters.script_encounter')
end

function ScriptEncounter:start(ctx, info)
   -- if there's a script associated with the mod, give it a chance to customize the camp
   if info.script then
      local script = radiant.create_controller(info.script, ctx, info.data)
      self._sv.script = script
      script:start(ctx, info.data)
   end

   ctx.arc:trigger_next_encounter(ctx)   
end

function ScriptEncounter:stop()
end

return ScriptEncounter