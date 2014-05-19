local ConstructionProgressClient = class()

function ConstructionProgressClient:initialize(entity)
   self._entity = entity
end

function ConstructionProgressClient:get_data()
   return self.__saved_variables:get_data()
end

function ConstructionProgressClient:get_teardown()
   return self:get_data().teardown
end

function ConstructionProgressClient:get_building_entity()
   return self:get_data().building_entity
end

function ConstructionProgressClient:begin_editing(building, fabricator)
   -- xxx: assert we're in a RW store...
   self.__saved_variables:modify_data(function (o)
         o.building_entity = building
         o.fabricator_entity = fabricator
      end)
end

return ConstructionProgressClient
