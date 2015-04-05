local mlc = require 'metalua.compiler'.new()
local FileMapper = {}

--[[
https://github.com/fab13n/metalua-parser

block: { stat* }

stat:
  `Do{ stat* }
| `Set{ {lhs+} {expr+} }                    -- lhs1, lhs2... = e1, e2...
| `While{ expr block }                      -- while e do b end
| `Repeat{ block expr }                     -- repeat b until e
| `If{ (expr block)+ block? }               -- if e1 then b1 [elseif e2 then b2] ... [else bn] end
| `Fornum{ ident expr expr expr? block }    -- for ident = e, e[, e] do b end
| `Forin{ {ident+} {expr+} block }          -- for i1, i2... in e1, e2... do b end
| `Local{ {ident+} {expr+}? }               -- local i1, i2... = e1, e2...
| `Localrec{ ident expr }                   -- only used for 'local function'
| `Goto{ <string> }                         -- goto str
| `Label{ <string> }                        -- ::str::
| `Return{ <expr*> }                        -- return e1, e2...
| `Break                                    -- break
| apply

expr:
  `Nil  |  `Dots  |  `True  |  `False
| `Number{ <number> }
| `String{ <string> }
| `Function{ { `Id{ <string> }* `Dots? } block }
| `Table{ ( `Pair{ expr expr } | expr )* }
| `Op{ opid expr expr? }
| `Stat{ block, expr } -- Only when Metalua extensions are loaded
| `Paren{ expr }       -- significant to cut multiple values returns
| apply
| lhs

apply:
  `Call{ expr expr* }
| `Invoke{ expr `String{ <string> } expr* }

lhs: `Id{ <string> } | `Index{ expr expr }

opid: 'add'   | 'sub'   | 'mul'   | 'div'
    | 'mod'   | 'pow'   | 'concat'| 'eq'
    | 'lt'    | 'le'    | 'and'   | 'or'
    | 'not'   | 'len'
]]--

function string:split(sSeparator, nMax, bRegexp)
   assert(sSeparator ~= '')
   assert(nMax == nil or nMax >= 1)

   local aRecord = {}

   if self:len() > 0 then
      local bPlain = not bRegexp
      nMax = nMax or -1

      local nField=1
      local nStart=1
      local nFirst, nLast = self:find(sSeparator, nStart, bPlain)
      while nFirst and nMax ~= 0 do
         aRecord[nField] = self:sub(nStart, nFirst-1)
         nField = nField+1
         nStart = nLast+1
         nFirst,nLast = self:find(sSeparator, nStart, bPlain)
         nMax = nMax-1
      end
      aRecord[nField] = self:sub(nStart)
   end

   return aRecord
end

local strval

local OPS = {
   add = '+',
   sub = '-',
   mul = '*',
   div = '/',
   mod = '%',
   pow = '^',
   concat = '..',
   eq = '==',
   lt = '<',
   le = '<=',
   ['and'] = 'and',
   ['or'] = 'or',
   ['not'] = 'not',
   len = '#',
}

local TO_STRING = {
   Id = function(expr) return expr[1] end,
   String = function(expr) return expr[1] end,
   Number = function(expr) return strval(expr[1]) end,
   Index = function(expr) return strval(expr[1]) .. '.' .. strval(expr[2]) end,
   Op = function(expr)
         local op, lhs, rhs = unpack(expr)
         if rhs then
            return strval(lhs) .. ' ' .. op .. ' '.. strval(rhs)
         end
         return op .. strval(lhs)
      end,
}

strval = function(e)
   if not e.tag then
      e.tag = 'xxx'
   end
   return TO_STRING[e.tag](e)
end

function FileMapper:map_source_function(file, filename)
   local ast = mlc:src_to_ast(file)
   local parts = filename:split('/')
   self._lines = {}
   self._symbols = {}
   self._shortname = parts[#parts]
   self._last_line = 0
   self:_msf_block(ast)

   if false then
      if filename:sub(1, 1) == '@' then
         filename = filename:sub(2)
      end
      filename = 'mods/' .. filename .. '.indexed'
      local f = io.open(filename, 'w')
      for i, line in pairs(file:split('\n')) do
         f:write(string.format('%50s | %s', self._lines[i] or ' ??? ', line))
      end
      f:close()
   end

   return self:_group_function_names()
end

function FileMapper:_group_function_names()
   local start = 1
   local current_symbol = self._lines[1] or '???'

   local group = {}
   for i=1,self._last_line do
      local symbol = self._lines[i] or '???'
      if symbol ~= current_symbol then
         table.insert(group, { current_symbol, start, i })
         current_symbol, start = symbol, i
      end
   end
   table.insert(group, { current_symbol, start, self._last_line })
   return group
end

function FileMapper:_add_lines_to_map(name, fn)
   local first = fn.lineinfo.first.line
   local last = fn.lineinfo.last.line

   local symbol_name = name and name or (self._shortname .. ' @ ' .. tostring(first))
   for i=first,last do
      self._lines[i] = symbol_name
      if name then
         self._symbols[i] = true
      end
   end
   self._last_line = math.max(self._last_line, last)
end

function FileMapper:_assign_function_to_local(lcl, expr)
   local function_name = strval(lcl)
   self:_add_lines_to_map(function_name, expr)
   self:_msf_block(expr[2])
end

function FileMapper:_assign_function_to_table(tbl, idx, expr)
   local sep = '.'
   local args = expr[1]
   if #args > 0 then
      local first_arg = args[1]
      if first_arg.tag == 'Id' and first_arg[1] == 'self' then
         sep = ':'
      end
   end

   local function_name = strval(tbl) .. sep .. strval(idx)
   self:_add_lines_to_map(function_name, expr)
   self:_msf_block(expr[2])
end

function FileMapper:_inspect_call_param(expr)
   if expr.tag == 'Function' then
      local lineno = expr.lineinfo.first.line
      local function_name = self._lines[lineno]
      if function_name and self._symbols[lineno] then
         self:_add_lines_to_map(function_name .. ' @ ' .. tostring(lineno), expr)
      else
         self:_add_lines_to_map(nil, expr)
      end
      self:_msf_block(expr[2])
   end
end

function FileMapper:_inspect_table(lhs, tbl)  
   -- `Table{ ( `Pair{ expr expr } | expr )* }
   for i, item in ipairs(tbl) do
      local idx, val
      if item.tag == 'Pair' then
         idx, val = item[1], item[2]
      elseif item.tag == 'Dots' then
      else
         idx, val = i, item[2]
      end
      if val and val.tag == 'Function' then
         self:_assign_function_to_table(lhs, idx, val)
      end
   end
end

function FileMapper:_inspect_set(lhs, expr)
   if expr then
      if expr.tag == 'Function' then
         -- `Function{ { `Id{ <string> }* `Dots? } block }
         if lhs.tag == 'Id' then
            self:_assign_function_to_local(lhs, expr)
         else
            assert(lhs.tag == 'Index')
            self:_assign_function_to_table(lhs[1], lhs[2], expr)
         end
      elseif expr.tag == 'Table' then
         self:_inspect_table(lhs, expr)
      elseif expr.tag == 'Paren' then
         -- `Paren{ expr }
         self:_inspect_set(lhs, expr[1])
      end
   end
end

function FileMapper:_msf_block(block)
   -- ipairs will ignore the tag
   for _, stat in ipairs(block) do 
      self:_msf_stat(stat)
   end
end

function FileMapper:_msf_stat(stat)
   self['_msf_' .. stat.tag .. 'Tag'](self, stat)
end

function FileMapper:_msf_DoTag(stat)
   self:_msf_block(stat)
end

function FileMapper:_msf_SetTag(stat)
   local lhs_s, expr_s = unpack(stat)
   for i, lhs in ipairs(lhs_s) do
      local expr = expr_s[i]
      self:_inspect_set(lhs, expr)
   end
end

function FileMapper:_msf_WhileTag(stat)
   local expr, block = unpack(stat)
   self:_msf_block(block)
end

function FileMapper:_msf_RepeatTag(stat)
   local block, expr = unpack(stat)
   self:_msf_block(block)
end

function FileMapper:_msf_IfTag(stat)
   -- (expr block)+ block?
   local c = #stat
   for i = 2, c, 2 do
      self:_msf_block(stat[i])
   end
   if math.fmod(c, 2) == 1 then
      self:_msf_block(stat[c])
   end
end

function FileMapper:_msf_FornumTag(stat)
   local block = stat[#stat]
   self:_msf_block(block)
end

function FileMapper:_msf_ForinTag(stat)
   local block = stat[#stat]
   self:_msf_block(block)
end

function FileMapper:_msf_LocalTag(stat)
   local lhs_s, expr_s = unpack(stat)
   if expr_s then
      for i, lhs in ipairs(lhs_s) do
         local expr = expr_s[i]
         if expr then
            self:_inspect_set(lhs, expr)
         end
      end
   end
end

function FileMapper:_msf_LocalrecTag(stat)
   local ident_s, expr_s = unpack(stat)
   for i=1,#ident_s do
      local ident, expr = ident_s[i], expr_s[i]
      self:_assign_function_to_local(ident, expr)
   end   
end

function FileMapper:_msf_GotoTag(stat)   
end

function FileMapper:_msf_LabelTag(stat)
end

function FileMapper:_msf_ReturnTag(stat)
end

function FileMapper:_msf_BreakTag(stat)
end

function FileMapper:_msf_CallTag(stat)
   -- Call{ expr expr* }
   for i=2,#stat do
      self:_inspect_call_param(stat[i])
   end
end

function FileMapper:_msf_InvokeTag(stat)
   -- `Invoke{ expr `String{ <string> } expr* }
   for i=3,#stat do
      self:_inspect_call_param(stat[i])
   end
end

return FileMapper
