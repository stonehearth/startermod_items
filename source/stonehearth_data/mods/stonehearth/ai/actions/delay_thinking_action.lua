local DelayThinking = class()

DelayThinking.name = 'delay thinking'
DelayThinking.does = 'stonehearth:delay_thinking'
DelayThinking.args = {
   time = 'number',     -- how many game ticks to delay
}
DelayThinking.version = 2
DelayThinking.priority = 1

function DelayThinking:start_thinking(ai, entity, args)
   self._active = true
   radiant.set_timer(radiant.gamestate.now() + args.time, function()
         if self._active then
            ai:set_think_output()
         end
      end)
end

function DelayThinking:stop_thinking()
   self._active = false
end

return DelayThinking
