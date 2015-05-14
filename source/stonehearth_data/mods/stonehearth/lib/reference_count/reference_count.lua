ReferenceCount = class()

function ReferenceCount:initialize()
   assert(self._sv)
   self._sv.ref_counts = {}
   self.__saved_variables:mark_changed()
end

function ReferenceCount:destroy()
end

function ReferenceCount:add_ref(key)
   local ref_count = self._sv.ref_counts[key] or 0
   ref_count = ref_count + 1
   self._sv.ref_counts[key] = ref_count
   self.__saved_variables:mark_changed()
   return ref_count
end

function ReferenceCount:dec_ref(key)
   local ref_count = self._sv.ref_counts[key] or 0
   ref_count = ref_count - 1

   if ref_count <= 0 then
      ref_count = 0
      self._sv.ref_counts[key] = nil
   else
      self._sv.ref_counts[key] = ref_count
   end

   self.__saved_variables:mark_changed()
   return ref_count
end

function ReferenceCount:clear()
   for key, ref_count in pairs(self._sv.ref_counts) do
      self._sv.ref_counts[key] = nil
   end
end

function ReferenceCount:get_all_refs()
   return self._sv.ref_counts
end

return ReferenceCount
