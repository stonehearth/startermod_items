local ActivationType = {}

ActivationType.immediate = 'immediate'
ActivationType.revealed = 'revealed'

function ActivationType.is_valid(value)
   return ActivationType[value] ~= nil
end

return ActivationType
