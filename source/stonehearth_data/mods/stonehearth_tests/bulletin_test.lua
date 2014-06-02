local MicroWorld = require 'lib.micro_world'
local BulletinTest = class(MicroWorld)

local count_1 = 1
local count_2 = 100

function BulletinTest:__init()
   self[MicroWorld]:__init()
   self:create_world()

   self:at(2000,
      function ()
         self._caravan_bulletin = stonehearth.bulletin_board:post_bulletin('player_1')
            :set_data('stonehearth:bulletins:caravan')
            :set_source(self)
      end
   )

   self:at(2500,
      function ()
         -- pretend the user accepted the dialog
         -- in javascript this would be:
         --    radiant.call_obj(source, 'accepted', args);
         local source = self._caravan_bulletin:get_source()
         source:accepted(314)
      end
   )

   self:at(4000,
      function ()
         stonehearth.bulletin_board:post_bulletin('player_1')
            :set_data('stonehearth:bulletins:goblin_thief')
      end
   )
end

function BulletinTest:accepted(param)
   local foo = param
end

return BulletinTest
