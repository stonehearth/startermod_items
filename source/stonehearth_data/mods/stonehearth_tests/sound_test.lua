local MicroWorld = require 'lib.micro_world'

local SoundTest = class(MicroWorld)

function SoundTest:__init()
   self[MicroWorld]:__init()
   self:create_world()

   local tree = self:place_tree(-12, -12)
   local dude = self:place_citizen(12, 12)

   radiant.music:play_music_effect("/stonehearth/effects/background_music/background_music.json")

   --TODO: it would be nice if we could play music from a track instead of pre-creating an effect for it
   --radiant.music:play_music("data/stonehearth/effects/background_music/soothing.ogg", true)

   ---[[
   --Some time in seconds in, play a different background music
   self:at(20000, function()
         radiant.music:play_music_effect("/stonehearth/effects/background_music/background_harp.json")
      end)
   --]]


end

return SoundTest