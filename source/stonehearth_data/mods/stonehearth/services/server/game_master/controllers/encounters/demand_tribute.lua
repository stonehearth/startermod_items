
local DemandTribute = class()

function DemandTribute:start(ctx, info)
   self._sv.info = info
end

return DemandTribute
