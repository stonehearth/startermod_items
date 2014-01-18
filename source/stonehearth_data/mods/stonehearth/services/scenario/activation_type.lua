local ActivationType = {}

ActivationType.static   = 'static'
ActivationType.revealed = 'revealed'

function ActivationType.is_valid(value)
   return ActivationType[value] ~= nil
end

return ActivationType
