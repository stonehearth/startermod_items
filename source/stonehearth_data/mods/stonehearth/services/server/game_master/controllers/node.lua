local Node = class()

local RARITY_VOTES = {
   common = 100,
   rare   = 50,
}

-- set the name of the node.  all nodes in a single nodelist must have unique
-- names
--
function Node:set_name(name)
   self._sv.name = name
   self.__saved_variables:mark_changed()
   return self
end

-- create a child nodelist of this node given the specified `list` data, which
-- is a table containing name to node-info file mappings.
--
function Node:_create_nodelist(expected_type, list)
   assert(type(list) == 'table', 'invalid nodelist passed to create_nodelist')

   return radiant.create_controller('stonehearth:game_master:node_list', self._sv.name, expected_type, list)
end

-- xxx: consider moving everything below to a different class

-- get the subtype of the node (e.g. an encounter node might have a subtype
-- of 'create_camp' or 'collection_quest')
--
function Node:get_subtype()
   assert(self._sv._info)
   return self._sv._info.subtype
end

-- get the rarity of the node
--
function Node:get_rarity()
   assert(self._sv._info)
   return self._sv._info.rarity
end

-- get the number of votes this node gets during the election
--
function Node:get_vote_count()
   local rarity = self:get_rarity() or 'common'
   return RARITY_VOTES[rarity] or 1
end

-- return whether or not the encounter is unique (i.e. cannot have more than 1 running instance)
--
function Node:get_is_unique()
   assert(self._sv._info)
   return self._sv._info.unique
end

return Node

