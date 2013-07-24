local BedComponent = class()

function BedComponent:__init(entity)
   self._entity = entity  -- the entity this component is attached to
end

return BedComponent