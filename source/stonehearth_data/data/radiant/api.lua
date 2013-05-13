-- the root of all evil =)

print 'loading radiant script!'

local builtin_require = require
require = function (lib)
	print ('you cannot require!! ' .. lib)
end
