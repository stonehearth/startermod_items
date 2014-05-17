local FabricatorClientComponent = class()

-- this is the component which manages the fabricator entity.
function FabricatorClientComponent:initialize(entity, json)
   self._entity = entity
end

function FabricatorClientComponent:get_data()
   return self.__saved_variables:get_data()
end

function FabricatorClientComponent:get_blueprint()
   return self:get_data().blueprint
end

function FabricatorClientComponent:get_project()
   return self:get_data().project
end

function FabricatorClientComponent:begin_editing(blueprint, project)
   -- xxx: assert we're in a RW store...
   self.__saved_variables:modify_data(function (o)
         o.blueprint = blueprint
         o.project = project
      end)
end

return FabricatorClientComponent
