local MicroWorld = require 'lib.micro_world'
local BulletinTest = class(MicroWorld)

local count_1 = 1
local count_2 = 100

function BulletinTest:__init()
   self[MicroWorld]:__init()
   self:create_world()

   --radiant.events.listen(radiant, 'stonehearth:very_slow_poll', self, self._post_test_bulletin)
   radiant.events.listen(radiant, 'stonehearth:very_slow_poll', self, self._update_bulletin)

   self:at(2000,
      function ()
         self._caravan_bulletin = stonehearth.bulletin_board:post_bulletin('player_1')
            :set_title('Caravan has arrived!')
            :set_callback('accepted', self, '_on_accepted')
      end
   )

   self:at(2500,
      function ()
         -- pretend the user accepted the dialog
         _radiant.call('stonehearth:trigger_bulletin_event', self._caravan_bulletin:get_id(), 'accepted', 314)
      end
   )

   self:at(4000,
      function ()
         stonehearth.bulletin_board:post_bulletin('player_1')
            :set_title('Goblins are stealing your wood!')
            :set_type('alert')
      end
   )

   self:at(6000,
      function ()
         self._countdown_bulletin = stonehearth.bulletin_board:post_bulletin('player_1')
            :set_title('Time left: ' .. count_2)
      end
   )
end

function BulletinTest:_on_accepted(param)
   local foo = param
end

function BulletinTest:_post_test_bulletin()
   stonehearth.bulletin_board:post_bulletin('player_1')
      :set_title('Bulletin ' .. count_1)

   count_1 = count_1 + 1
end

function BulletinTest:_update_bulletin()
   if self._countdown_bulletin then
      count_2 = count_2 - 1
      self._countdown_bulletin:set_title('Time left: ' .. count_2)
      --stonehearth.bulletin_board:remove_bulletin(self._countdown_bulletin:get_id())
   end
end

return BulletinTest
