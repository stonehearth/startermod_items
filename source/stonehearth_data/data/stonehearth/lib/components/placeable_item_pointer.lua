--[[
   Placeable_item_pointer.lua
   When an item needs to be shrunk to a proxy in order
   to be picked up and moved, it needs to remember what
   that proxy object is. This component keeps track of that information.
]]

local PlaceableItemPointer = class()

function PlaceableItemPointer:__init(entity)
   self._entity = entity
   self._proxy_entity = nil               --the 1x1 entity that we need to shrink to to be carried, etc
   
end

function PlaceableItemPointer:extend(json)
end

function PlaceableItemPointer:set_proxy(proxy_entity)
   self._proxy_entity = proxy_entity
end

function PlaceableItemPointer:get_proxy()
   return self._proxy_entity
end

return PlaceableItemPointer