local Point3 = _radiant.csg.Point3
local Entity = _radiant.om.Entity
local FollowPartyOrders = class()

FollowPartyOrders.name = 'party orders'
FollowPartyOrders.does = 'stonehearth:follow_party_orders'
FollowPartyOrders.args = {
   party = 'table' -- the Party object, in truth
}
FollowPartyOrders.version = 2
FollowPartyOrders.priority = 1

local ai = stonehearth.ai
return ai:create_compound_action(FollowPartyOrders)
         :execute('stonehearth:party:orders', { party = ai.ARGS.party })

