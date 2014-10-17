local MicroWorld = require 'lib.micro_world'

local SoundTest = class(MicroWorld)

function SoundTest:__init()
   self[MicroWorld]:__init()
   self:create_world()

   local tree = self:place_tree(-12, -12)
   local dude = self:place_citizen(12, 12)

   --TODO: move to client-side, for example, sky-renderer would be able to call day/night effects
   --TODO: also vary based on camera position
   --Thus this test no longer works. If you are in client-side lua, you can call _radiant.call('stonehearth:play_music', {args})
   --but since play_music is in client.cpp, you cannot call it from the server. 

   _radiant.audio.play_music('stonehearth:music:title_screen', 'bgm')

   ---[[
   --Some time in seconds in, play a different background music
   self:at(20000, function()
         _radiant.audio.play_music('stonehearth:music:world_start', 'bgm')
      end)
   --]]


end

return SoundTest