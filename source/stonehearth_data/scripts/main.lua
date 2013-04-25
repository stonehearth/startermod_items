require 'package'
local block = package.loadlib('c:/Users/ponder/gamedev/radiant/projects/tesseract/build/client_dll/RelWithDebInfo/client_dll.dll', 'init')
--local block = package.loadlib('c:/Users/ponder/gamedev/radiant/projects/tesseract/build/client_dll/Debug/client_dll.dll', 'init')
print('got '..tostring(block)..' from package.loadlib');
if block then
   block()
   start(arg)
end
   
while true do
end
