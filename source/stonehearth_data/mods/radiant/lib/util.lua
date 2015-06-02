local util = {}

--[[
  Was failing whenever the object was a RadiantI3 because obj:is_valid() was undefined.
]]

function util.is_string(obj)
   return type(obj) == 'string'
end

function util.is_number(obj)
   return type(obj) == 'number'
end

function util.colorcode_to_integer(cc)
   local result = 0
   if type(cc) == 'string' then
      -- parse the digits out of 'rgb(13, 34, 19)', allowing for spaces between tokens
      local r, g, b = cc:match('rgb%s*%(%s*(%d+)%s*,%s*(%d+)%s*,%s*(%d+)%s*%)')
      if r and g and b then
         result = (((tonumber(b) * 256) + tonumber(g)) * 256) + tonumber(r)
      end
   end
   return result
end

function util.equals(value)
end

function util.tostring(value)
   local t = type(value)
   if t == 'userdata' then
      local v = tostring(value)
      if true then
         return v
      end
      if value.get_type_name then
         return value:get_type_name()
      end
      return class_info(value).name
   elseif util.is_class(value) then
      return value.__classname or '*anonymous class*'
   elseif util.is_instance(value) then
      return util.tostring(value.__class)
   end
   return tostring(value)
end

function util.typename(value)
   local t = type(value)
   if t == 'userdata' then
      if value.get_type_name then
         return value:get_type_name()
      end
      return class_info(value).name
   end
   if t == 'table' then
      if util.is_class(value) then
         return value.__classname or '*anonymous class*'
      end
      if util.is_instance(value) then
         return util.tostring(value.__class)
      end
   end
   return t
end

function util.is_instance(maybe_instance)
   return type(maybe_instance) == 'table' and maybe_instance.__type == 'object'
end

function util.is_class(maybe_cls)
   return type(maybe_cls) == 'table' and maybe_cls.__type == 'class'
end

function util.table_is_empty(t)
   return #t == 0 and next(t) == nil
end

function util.is_a(var, cls)
   local t = type(var)
   if t == 'userdata' then
      if type(cls) ~= 'userdata' then
         return false
      end
      return var:get_type_id() == cls.get_type_id()
   elseif util.is_instance(var) and util.is_class(cls) then
      return var:is_a(cls)
   end
   return t == cls
end

function util.is_datastore(datastore)
   return radiant.util.is_a(datastore, _radiant.om.DataStore) or
          radiant.util.is_a(datastore, _radiant.om.DataStoreRefWrapper)
end

function util.get_config(str, default)
   -- The stack offset for the helper functions is 3...
   --    1: __get_current_module_name
   --    2: util.get_config       
   --    3: --> some module whose name we want! <-- 
   local modname = __get_current_module_name(3)
   local value = _host:get_config('mods.' .. modname .. "." .. str)
   return value == nil and default or value
end

--- Split a string into a table (e.g. "foo bar baz" -> { "foo", "bar", "baz" }
-- Borrowed from http://lua-users.org/wiki/SplitJoin
function util.split_string(str, sep)
   -- esacpe all special chars (for now, just .  someone find the rest!)
   if sep == nil then
      sep = ' '
   elseif sep == '.' then
      sep = '%' .. sep
   end
   local ret={}
   local n=1
   for w in str:gmatch("([^"..sep.."]*)") do
      ret[n]=ret[n] or w -- only set once (so the blank after a string is ignored)
      if w=="" then n=n+1 end -- step forwards on a blank but not a string
    end
   return ret
end

-- From https://github.com/davidm/lua-inspect/blob/master/metalualib/metalua/table2.lua
-- Converts a lua table into string format for logging/debugging.
function util.table_tostring(t, ...)
   local PRINT_HASH, HANDLE_TAG, FIX_INDENT, LINE_MAX, INITIAL_INDENT = true, true
   for _, x in ipairs {...} do
      if type(x) == "number" then
         if not LINE_MAX then LINE_MAX = x
         else INITIAL_INDENT = x end
      elseif x=="nohash" then PRINT_HASH = false
      elseif x=="notag"  then HANDLE_TAG = false
      else
         local n = string['match'](x, "^indent%s*(%d*)$")
         if n then FIX_INDENT = tonumber(n) or 3 end
      end
   end
   LINE_MAX       = LINE_MAX or math.huge
   INITIAL_INDENT = INITIAL_INDENT or 1
   
   local current_offset =  0  -- indentation level
   local xlen_cache     = { } -- cached results for xlen()
   local acc_list       = { } -- Generated bits of string
   local function acc(...)    -- Accumulate a bit of string
      local x = table.concat{...}
      current_offset = current_offset + #x
      table.insert(acc_list, x) 
   end
   local function valid_id(x)
      -- FIXME: we should also reject keywords; but the list of
      -- current keywords is not fixed in metalua...
      return type(x) == "string" 
         and string['match'](x, "^[a-zA-Z_][a-zA-Z0-9_]*$")
   end
   
   -- Compute the number of chars it would require to display the table
   -- on a single line. Helps to decide whether some carriage returns are
   -- required. Since the size of each sub-table is required many times,
   -- it's cached in [xlen_cache].
   local xlen_type = { }
   local function xlen(x, nested)
      nested = nested or { }
      if x==nil then return #"nil" end
      --if nested[x] then return #tostring(x) end -- already done in table
      local len = xlen_cache[x]
      if len then return len end
      local f = xlen_type[type(x)]
      if not f then return #tostring(x) end
      len = f (x, nested) 
      xlen_cache[x] = len
      return len
   end

   -- optim: no need to compute lengths if I'm not going to use them
   -- anyway.
   if LINE_MAX == math.huge then xlen = function() return 0 end end

   xlen_type["nil"] = function () return 3 end
   function xlen_type.number  (x) return #tostring(x) end
   function xlen_type.boolean (x) return x and 4 or 5 end
   function xlen_type.string  (x) return #string.format("%q",x) end
   function xlen_type.table   (adt, nested)

      -- Circular references detection
      if nested [adt] then return #tostring(adt) end
      nested [adt] = true

      local has_tag  = HANDLE_TAG and valid_id(adt.tag)
      local alen     = #adt
      local has_arr  = alen>0
      local has_hash = false
      local x = 0
      
      if PRINT_HASH then
         -- first pass: count hash-part
         for k, v in pairs(adt) do
            if k=="tag" and has_tag then 
               -- this is the tag -> do nothing!
            elseif type(k)=="number" and k<=alen and math.fmod(k,1)==0 then 
               -- array-part pair -> do nothing!
            else
               has_hash = true
               if valid_id(k) then x=x+#k
               else x = x + xlen (k, nested) + 2 end -- count surrounding brackets
               x = x + xlen (v, nested) + 5          -- count " = " and ", "
            end
         end
      end

      for i = 1, alen do x = x + xlen (adt[i], nested) + 2 end -- count ", "
      
      nested[adt] = false -- No more nested calls

      if not (has_tag or has_arr or has_hash) then return 3 end
      if has_tag then x=x+#adt.tag+1 end
      if not (has_arr or has_hash) then return x end
      if not has_hash and alen==1 and type(adt[1])~="table" then
         return x-2 -- substract extraneous ", "
      end
      return x+2 -- count "{ " and " }", substract extraneous ", "
   end
   
   -- Recursively print a (sub) table at given indentation level.
   -- [newline] indicates whether newlines should be inserted.
   local function rec (adt, nested, indent)
      if not FIX_INDENT then indent = current_offset end
      local function acc_newline()
         acc ("\n"); acc (string.rep (" ", indent)) 
         current_offset = indent
      end
      local x = { }
      x["nil"] = function() acc "nil" end
      function x.number()   acc (tostring (adt)) end
      --function x.string()   acc (string.format ("%q", adt)) end
      function x.string()   acc ((string.format ("%q", adt):gsub("\\\n", "\\n"))) end
      function x.boolean()  acc (adt and "true" or "false") end
      function x.table()
         if nested[adt] then acc(tostring(adt)); return end
         nested[adt]  = true


         local has_tag  = HANDLE_TAG and valid_id(adt.tag)
         local alen     = #adt
         local has_arr  = alen>0
         local has_hash = false

         if has_tag then acc("`"); acc(adt.tag) end

         -- First pass: handle hash-part
         if PRINT_HASH then
            for k, v in pairs(adt) do
               -- pass if the key belongs to the array-part or is the "tag" field
               if not (k=="tag" and HANDLE_TAG) and 
                  not (type(k)=="number" and k<=alen and math.fmod(k,1)==0) then

                  -- Is it the first time we parse a hash pair?
                  if not has_hash then 
                     acc "{ "
                     if not FIX_INDENT then indent = current_offset end
                  else acc ", " end

                  -- Determine whether a newline is required
                  local is_id, expected_len = valid_id(k)
                  if is_id then expected_len = #k + xlen (v, nested) + #" = , "
                  else expected_len = xlen (k, nested) + 
                                      xlen (v, nested) + #"[] = , " end
                  if has_hash and expected_len + current_offset > LINE_MAX
                  then acc_newline() end
                  
                  -- Print the key
                  if is_id then acc(k); acc " = " 
                  else  acc "["; rec (k, nested, indent+(FIX_INDENT or 0)); acc "] = " end

                  -- Print the value
                  rec (v, nested, indent+(FIX_INDENT or 0))
                  has_hash = true
               end
            end
         end

         -- Now we know whether there's a hash-part, an array-part, and a tag.
         -- Tag and hash-part are already printed if they're present.
         if not has_tag and not has_hash and not has_arr then acc "{ }"; 
         elseif has_tag and not has_hash and not has_arr then -- nothing, tag already in acc
         else 
            assert (has_hash or has_arr)
            local no_brace = false
            if has_hash and has_arr then acc ", " 
            elseif has_tag and not has_hash and alen==1 and type(adt[1])~="table" then
               -- No brace required; don't print "{", remember not to print "}"
               acc (" "); rec (adt[1], nested, indent+(FIX_INDENT or 0))
               no_brace = true
            elseif not has_hash then
               -- Braces required, but not opened by hash-part handler yet
               acc "{ "
               if not FIX_INDENT then indent = current_offset end
            end

            -- 2nd pass: array-part
            if not no_brace and has_arr then 
               rec (adt[1], nested, indent+(FIX_INDENT or 0))
               for i=2, alen do 
                  acc ", ";                   
                  if   current_offset + xlen (adt[i], { }) > LINE_MAX
                  then acc_newline() end
                  rec (adt[i], nested, indent+(FIX_INDENT or 0)) 
               end
            end
            if not no_brace then acc " }" end
         end
         nested[adt] = false -- No more nested calls
      end
      local y = x[type(adt)]
      if y then y() else acc(tostring(adt)) end
   end
   --printf("INITIAL_INDENT = %i", INITIAL_INDENT)
   current_offset = INITIAL_INDENT or 0
   rec(t, { }, 0)
   return table.concat (acc_list)
end

-- when a check fails, use util.typename to get the type of the
-- argument
checkers.__get_typename = util.typename

return util
