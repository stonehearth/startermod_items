-- replace the global assert
assert = function(condition)   
   if not condition then
      native:assert_failed('assertion failed')
   end
   return condition
end

require 'radiant.lualibs.unclasslib'

-- finish off by locking down the global namespace
require 'radiant.lualibs.strict'
decoda_name = "main"