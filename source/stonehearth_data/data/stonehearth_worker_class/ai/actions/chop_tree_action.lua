local ChopTreeAction = class()

ChopTreeAction.name = 'stonehearth.actions.chop_tree'
ChopTreeAction.does = 'stonehearth.activities.top'
ChopTreeAction.priority = 0


function ChopTreeAction:__init(ai, entity)
   self._ai = ai
   self._entity = entity
   radiant.events.listen('radiant.events.gameloop', self)
end

ChopTreeAction['radiant.events.gameloop'] = function(self)
   local faction = self._entity:get_component('unit_info'):get_faction()
   local inventory = radiant.mods.require('mod://stonehearth_inventory').get_inventory(faction)

   inventory:find_path_to_tree(self._entity, function(path)
         assert(self._entity:get_id() == path:get_entity():get_id())
         self._path = path
         self._ai:set_action_priority(self, 10)
      end)
      
   radiant.events.unlisten('radiant.events.gameloop', self)
end

function ChopTreeAction:run(ai, entity)
   assert(self._path)
   local tree = self._path:get_destination():get_entity()

   ai:execute('stonehearth.activities.follow_path', self._path)
   radiant.entities.turn_to_face(entity, tree)
   while true do
      ai:execute('stonehearth.activities.run_effect', 'chop')
   end
   
   -- spawn the tree...
   assert(false)

   self._path = nil
   ai:set_priority(self, 0)
end

return ChopTreeAction