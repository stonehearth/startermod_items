local FarmerFieldLayerComponent = class()

function FarmerFieldLayerComponent:set_farmer_field(field)
   self._sv.farmer_field = field
   self.__saved_variables:mark_changed()
end

function FarmerFieldLayerComponent:get_farmer_field()
   return self._sv.farmer_field
end

return FarmerFieldLayerComponent
