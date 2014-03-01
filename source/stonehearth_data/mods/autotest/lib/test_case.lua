local AutoTest = class()

function AutoTest:__init(testfn)
   self._testfn = testfn
end

function AutoTest:run()
   self._testfn(self)
end

return AutoTest