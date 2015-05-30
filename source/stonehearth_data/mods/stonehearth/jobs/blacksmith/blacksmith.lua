local CraftingJob = require 'jobs.crafting_job'

local BlacksmithClass = class()
radiant.mixin(BlacksmithClass, CraftingJob)

return BlacksmithClass
