--This class governs the background music playing in the game
--TODO: a.) fade one music out as another comes in
--      b.) create a music effect from a filename

local BgmManager = class()

function BgmManager:__init()
   self._curr_music_effect = nil
   radiant.entities.get_root_entity():add_component('effect_list')
end

--[[
--TODO: make a function that takes the trackname and loop info and creates an effect for it
function BgmManager:play_music(track_name, loop)
   --Would be nice to play from a song name
   --local effect_string = '{"type" : "effect", "tracks": { "music": {"type": "music_effect","start_time": 0,"track" : "' .. track_name ..'","loop": ' .. tostring(loop) .. '}}}'
   self:stop_curr_music()
   self._curr_music_effect =
      radiant.effects.run_effect(radiant.entities.get_root_entity(), effect_string)
end
]]

-- Play the passed-in music effect. Stop any currently running music effect
function BgmManager:play_music_effect(effect_uri)
   self:stop_current()
   self._curr_music_effect =
      radiant.effects.run_effect(radiant.entities.get_root_entity(), effect_uri)
end

-- If there is currently music running, stop it
function BgmManager:stop_current()
   if self._curr_music_effect then
      --TODO: fade first
      self._curr_music_effect:stop()
   end
end

return BgmManager