--[[---------------
Utf8 v0.4
-------------------
utf8 -> unicode ucs2 converter

How to use:
to convert:
 ucs2_string = utf8.utf_to_uni(utf8_string)
 
to view a string in hex:
 utf8.print_hex(str)

Under the MIT license.

Utf8 is a part of LuaBit Project(http://luaforge.net/projects/bit/).

copyright(c) 2007 hanzhao (abrash_han@hotmail.com)
--]]---------------


require '/cd/lua/lib/bit/bit.lua';
require '/cd/lua/lib/bit/hex.lua';


do
 local BYTE_1_HEAD = hex.to_dec('0x00') -- 0### ####
 local BYTE_2_HEAD = hex.to_dec('0xC0') -- 110# ####
 local BYTE_3_HEAD = hex.to_dec('0xE0') -- 1110 ####

 -- mask to get the head
 local BYTE_1_MASK = hex.to_dec('0x80') -- 1### ####
 local BYTE_2_MASK = hex.to_dec('0xE0') -- 111# ####
 local BYTE_3_MASK = hex.to_dec('0xF0') -- 1111 ####
 
 -- tail byte mask
 local TAIL_MASK = hex.to_dec('0x3F') -- 10## ####

 local mask_tbl = {
  BYTE_3_MASK,
  BYTE_2_MASK,
  BYTE_1_MASK,
 }
 local head_tbl = {
  BYTE_3_HEAD,
  BYTE_2_HEAD,
  BYTE_1_HEAD,
 }
 
 local len_tbl = {
  [BYTE_1_HEAD] = 1,
  [BYTE_2_HEAD] = 2,
  [BYTE_3_HEAD] = 3,
 }

 local function utf_read_char(utf, start)
  local head_byte = string.byte(utf, start)
  --print('head byte ' .. hex.to_hex(head_byte))
  for m = 1, table.getn(mask_tbl) do
   local mask = mask_tbl[m]
   -- head match
   local head = bit.band(head_byte, mask)
   --print('head ' .. hex.to_hex(head) .. ' ' .. hex.to_hex(mask))
   if(head == head_tbl[m]) then
    local len = len_tbl[head_tbl[m]]
    --print('len ' .. len)
    local tail_idx = start + len - 1
    local char = 0
    -- tail
    for i = tail_idx, start + 1, -1 do
     local tail_byte = string.byte(utf, i)
     local byte = bit.band(tail_byte, TAIL_MASK)
     --print('byte ' .. hex.to_hex(byte).. ' = ' .. hex.to_hex(tail_byte) .. '&'..hex.to_hex(TAIL_MASK))
     if(tail_idx - i > 0) then
      local sft = bit.blshift(byte, (tail_idx - i) * 6)
      --print('shift ' .. hex.to_hex(sft) .. ' ' .. hex.to_hex(byte) .. ' ' .. ((tail_idx - i) * 6))
      char = bit.bor(char, sft)
      --print('char ' .. hex.to_hex(char))
     else
      char = byte
     end
    end -- tails
    
    -- add head
    local head_val = bit.band(head_byte, bit.bnot(mask))
    --print('head val ' .. hex.to_hex(head_val))
    head_val = bit.blshift(head_val, (len-1) * 6)
    --print('head val ' .. hex.to_hex(head_val))
    char = bit.bor(head_val, char)
    --print('char ' .. hex.to_hex(char))
    
    return char, len
   end -- if head match
  end -- for mask
  error('not find proper head mask')
 end
 
 local function print_hex(str)
  local cat = ''
  for i=1, string.len(str) do
   cat = cat .. ' ' .. hex.to_hex(string.byte(str, i))
  end
  print(cat)
 end

 local HI_MASK = hex.to_dec('0xF0')
 local LO_MASK = hex.to_dec('0xFF')
 
 local function char_to_str(char)
  local hi, lo = bit.brshift(char, 8), bit.band(char, LO_MASK)
  -- print(hex.to_hex(char)..' '..hex.to_hex(hi)..' ' .. hex.to_hex(lo))
  if(hi == 0) then
   return string.format('%c\0', lo)
  elseif(lo == 0) then
   return string.format('\0%c', hi)
  else
   return string.format('%c%c', lo, hi)
  end
 end
 
 local function utf_to_uni(utf)
  local n = string.len(utf)
  local i = 1
  local uni = ''
  while(i <= n) do
   --print('---')
   char, len = utf_read_char(utf, i)
   i = i + len
   --print(string.len(char_to_str(char)))
   
   uni = uni..char_to_str(char)
  end
  --print_hex(uni)
  return uni
 end

 -- interface
 utf8 = {
  utf_to_uni = utf_to_uni,
  print_hex = print_hex,
 }

end

--[[
-- test
byte_3 = string.format('%c%c%c', hex.to_dec('0xE7'), hex.to_dec('0x83'), hex.to_dec('0xad'))
print(string.len(byte_3))
utf8.utf_to_uni(byte_3)
--]]
--[[
byte_2 = string.format('%c%c', hex.to_dec('0xC2'), hex.to_dec('0x9D'))
utf8.utf_to_uni(byte_2)

byte_1 = string.format('%c', hex.to_dec('0xB'))
utf8.utf_to_uni(byte_1)
--]]
--[[
test_mul = string.format(
'%c%c%c%c%c%c%c%c%c',
hex.to_dec('0xE8'),hex.to_dec('0xAF'), hex.to_dec('0xBA'),
hex.to_dec('0xE5'),hex.to_dec('0x9F'), hex.to_dec('0xBA'),
hex.to_dec('0xE4'),hex.to_dec('0xBA'), hex.to_dec('0x9A'))

utf8.print_hex(utf8.utf_to_uni(test_mul))
--]]