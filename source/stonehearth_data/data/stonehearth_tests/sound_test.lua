local MicroWorld = require 'stonehearth_tests.lib.micro_world'

local SoundTest = class(MicroWorld)

function SoundTest:__init()
   self[MicroWorld]:__init()
   self:create_world()

   local tree = self:place_tree(-12, -12)
   local dude = self:place_citizen(12, 12)

   radiant.entities.get_root_entity():add_component('effect_list')
   radiant.effects.run_effect(radiant.entities.get_root_entity(), "/stonehearth/effects/background_music/background_music.json")

end

return SoundTest