local MicroWorld = require 'lib.micro_world'

local SoundTest = class(MicroWorld)

function SoundTest:__init()
   self[MicroWorld]:__init()
   self:create_world()

   local tree = self:place_tree(-12, -12)
   local dude = self:place_citizen(12, 12)

   --REVIEW QUESTION: Couldn't make this work... how to pass json from Lua?
   --Otherwise, would have to pass all the variables
   --local json = {
   --   track = 'stonehearth:music:title_screen'
   --}
   --_radiant.audio.play_music(radiant.json.encode(json))
   _radiant.audio.play_music('stonehearth:music:title_screen', true, 500, 50, 'bgm')

   ---[[
   --Some time in seconds in, play a different background music
   self:at(20000, function()
         _radiant.audio.play_music('stonehearth:music:world_start', true, 500, 50, 'bgm')
      end)
   --]]


end

return SoundTest