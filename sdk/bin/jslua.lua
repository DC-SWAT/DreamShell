#! /usr/local/bin/lua-5.1
-- 
-- * jslua - transform javascript like syntax into lua
-- *
-- * @author : Vincent Penne (ziggy at zlash.com)
-- *
-- *
if arg [ 0 ] then
   standalone = 1 
end
format = string . format 
function enter_namespace(name )
   local parent = getfenv ( 2 )
   local namespace = { parent_globals = parent }
   local i , v 
   for i , v in pairs ( parent ) do
      if i ~= "parent_globals" then
         namespace [ i ]= v 
      end
   end
   parent [ name ]= namespace 
   namespace [ name ]= namespace 
   setfenv ( 2 , namespace )
end

function exit_namespace()
   local parent = getfenv ( 2 )
   setfenv ( 2 , parent . parent_globals )
end

enter_namespace ( "jslua" )
local symbols = "~!#%%%^&*()%-%+=|/%.,<>:;\"'%[%]{}%?" 
local symbolset = "[" .. symbols .. "]" 
local msymbols = "<>!%&%*%-%+%=%|%/%%%^%." 
local msymbolset = "[" .. msymbols .. "]" 
local nospace = { [ "~" ] = 1 , [ "#" ] = 1 , }
local paths = { "" , }
function add_path(path )
   table . insert ( paths , path )
end

local loadfile_orig = loadfile 
function loadfile(name )
   local module , error 
   for _ , path in pairs ( paths ) do
      module , error = loadfile_orig ( path .. name )
      if module then
         setfenv ( module , getfenv ( 2 ) )
         break 
      end
   end
   return module , error 
end

function _message(msg )
   io . stderr : write ( msg .. "\n" )
end

function message(msg )
   if verbose then
      _message ( msg )
   end
end

function dbgmessage(msg )
   if dbgmode then
      _message ( msg )
   end
end

function emiterror(msg , source )
   local p = "" 
   source = source or cursource 
   if source then
      p = format ( "%s (%d) at token '%s' : " , source . filename , source . nline or - 1 , source . token or "<null>" )
   end
   _message ( p .. msg )
   has_error = 1 
   num_error = ( num_error or 0 )+ 1 
end

function colapse(t )
   local i , n 
   while t [ 2 ] do
      n = #t 
      local t2 = { }
      for i = 1 , n , 2 do
         table . insert ( t2 , t [ i ] .. ( t [ i + 1 ] or "" ) )
      end
      t = t2 
   end
   return t [ 1 ]or "" 
end

function opensource(realfn , filename )
   local source = { }
   source . filename = filename 
   if standalone then
      if realfn then
         source . handle = io . open ( filename , "r" )
      else
         source . handle = io . stdin 
      end
      if not source . handle then
         emiterror ( format ( "Can't open source '%s'" , filename ) )
         return nil 
      end
   else
      source . buffer = gBuffer 
      source . bufpos = 1 
   end
   source . nline = 0 
   source . ntoken = 0 
   source . tokens = { }
   source . tokentypes = { }
   source . tokenlines = { }
   source . tokencomments = { }
   local token = basegettoken ( source )
   while token do
      table . insert ( source . tokens , token )
      table . insert ( source . tokentypes , source . tokentype )
      table . insert ( source . tokenlines , source . nline )
      source . ntoken = source . ntoken + 1 
      token = basegettoken ( source )
   end
   source . tokenpos = 1 
   return source 
end

function closesource(source )
   if standalone then
      source . handle : close ( )
      source . handle = nil 
   end
   cursource = nil 
end

function getline(source )
   if standalone then
      source . linebuffer = source . handle : read ( "*l" )
   else
      if not source . bufpos then
         return 
      end
      local i = string . find ( source . buffer , "\n" , source . bufpos )
      source . linebuffer = string . sub ( source . buffer , source . bufpos , ( i or 0 ) - 1 )
      source . bufpos = i and ( i + 1 )
   end
   source . nline = source . nline + 1 
   source . newline = 1 
end

function savepos(source )
   return source . tokenpos 
end

function gotopos(source , pos )
   source . tokenpos = pos - 1 
   return gettoken ( source )
end

function gettoken(source )
   cursource = source 
   local pos = source . tokenpos 
   local token = source . tokens [ pos ]
   source . token = token 
   source . tokentype = source . tokentypes [ pos ]
   source . nline = source . tokenlines [ pos ]
   if not token then
      return 
   end
   source . tokenpos = pos + 1 
   dbgmessage ( token )
   if source . tokentype == "comment" then
      if not source . tokencomments [ source . tokenpos - 1 ] then
         out ( string . gsub ( "-- " .. token , "\n" , outcurindent .. "\n--" ) .. "\n" )
         source . tokencomments [ source . tokenpos - 1 ]= 1 
      end
      return gettoken ( source )
   end
   return token 
end

function basegettoken(source )
   local newline 
   local tokens 
   if not source . linebuffer then
      newline = 1 
      getline ( source )
      if not source . linebuffer then
         return nil 
      end
   else
      source . newline = nil 
   end
   local i , j 
   local s = source . linebuffer 
   i = string . find ( s , "%S" )
   if not i then
      source . linebuffer = nil 
      return basegettoken ( source )
   end
   j = string . find ( s , "[%s" .. symbols .. "]" , i )
   if not j then
      j = string . len ( s )+ 1 
   else
      j = j - 1 
   end
   source . stick = ( i == 1 )
   source . tokentype = "word" 
   if i ~= j then
      local c = string . sub ( s , i , i )
      if string . find ( c , symbolset ) then
         j = i 
         while string . find ( string . sub ( s , j + 1 , j + 1 ) , msymbolset ) and string . find ( c , msymbolset ) do
            j = j + 1 
         end
         source . tokentype = string . sub ( s , i , j )
      else
         if string . find ( string . sub ( s , j - 1 , j ) , symbolset ) then
            j = j - 1 
         end
      end
   end
   token = string . sub ( s , i , j )
   source . token = token 
   source . linebuffer = string . sub ( s , j + 1 )
   if token == "\"" or token == "'" then
      local t = token 
      s = source . linebuffer 
      local ok 
      while not ok do
         local _ , k 
         _ , k = string . find ( s , t )
         while k and k > 1 do
            local l = k - 1 
            local n = 0 
            while l > 0 and string . sub ( s , l , l ) == "\\" do
               l = l - 1 
               n = n + 0.5 
            end
            if n > 0 then
               dbgmessage ( format ( "N = %g (%g) '%s'" , n , math . floor ( n ) , source . linebuffer ) )
            end
            if math . floor ( n ) == n then
               break 
            end
            _ , k = string . find ( s , t , k + 1 )
         end
         if k then
            token = token .. string . sub ( s , 1 , k )
            source . linebuffer = string . sub ( s , k + 1 )
            dbgmessage ( format ( "TOKEN(%s) REST(%s), k(%d)" , token , source . linebuffer , k + 1 ) )
            ok = 1 
         else
            token = token .. string . sub ( s , 1 , - 2 )
            getline ( source )
            if not source . linebuffer then
               return nil 
            end
            s = source . linebuffer 
         end
      end
      source . tokentype = t 
   end
   if token == "//" or ( source . newline and token == "#" ) then
      getline ( source )
      return basegettoken ( source )
   end
   if token == "/*" then
      local _ , k 
      _ , k = string . find ( s , "*/" , j + 2 )
      token = "" 
      while not k do
         token = token .. string . sub ( s , j + 2 ).. "\n" 
         getline ( source )
         s = source . linebuffer 
         if not s then
            return nil 
         end
         _ , k = string . find ( s , "*/" )
         j = - 1 
      end
      source . linebuffer = string . sub ( s , k + 1 )
      source . tokentype = "comment" 
      token = token .. string . sub ( s , j + 2 , k - 1 )
   end
   if source . tokentype == "word" and not string . find ( token , "[^0123456789%.]" ) then
      source . tokentype = "number" 
      local s = source . linebuffer 
      if string . sub ( s , 1 , 1 ) == "." then
         local i = string . find ( s , "%D" , 2 )
         if i then
            source . linebuffer = string . sub ( s , i )
            i = i - 1 
         else
            source . linebuffer = nil 
         end
         token = token .. string . sub ( s , 1 , i )
      end
   end
   return token 
end

local exprstack = { }
function processaccum(source , token , what )
   out ( "= " )
   for i = exprstack [ #exprstack ] - 1 , source . tokenpos - 3 , 1 do
      out ( source . tokens [ i ] .. " " )
   end
   out ( string . sub ( what , 1 , 1 ) .. " " )
   return token 
end

function processincr(source , token , what )
   processaccum ( source , token , what )
   out ( "1 " )
   return token 
end

local exprkeywords = { [ "function" ] = function (source , token , what )
   out ( "function " )
   if token ~= "(" then
      out ( token )
      token = gettoken ( source )
   end
   if token ~= "(" then
      emiterror ( "'(' expected" , source )
      return token 
   end
   out ( "(" )
   token = processblock ( source , "(" , ")" , 1 )
   out ( ")" )
   outnl ( )
   outindent ( 1 )
   token = processstatement ( source , gettoken ( source ) , 1 )
   outindent ( - 1 )
   outi ( )
   out ( "end" )
   outnl ( )
   gotopos ( source , source . tokenpos - 1 )
   return ";" 
end
, [ "var" ] = "local" , [ "||" ] = "or" , [ "&&" ] = "and" , [ "!=" ] = "~=" , [ "!" ] = "not" , [ "+=" ] = processaccum , [ "-=" ] = processaccum , [ "*=" ] = processaccum , [ "/=" ] = processaccum , [ "++" ] = processincr , [ "--" ] = processincr , }
function eatexpr(source , token )
   while token and exprkeywords [ token ] do
      local expr = exprkeywords [ token ]
      if type ( expr ) == "string" then
         out ( expr .. " " )
         token = gettoken ( source )
      else
         token = exprkeywords [ token ]( source , gettoken ( source ) , token )
      end
   end
   return token 
end

function processblock(source , open , close , n )
   if n < 1 then
      if gettoken ( source ) ~= open then
         emiterror ( format ( "expected '%s' but got '%s'" , open , source . token ) , source )
         return nil 
      end
      n = 1 
   end
   local token = gettoken ( source )
   table . insert ( exprstack , savepos ( source ) )
   while n >= 1 do
      token = eatexpr ( source , token )
      if not token then
         return nil 
      end
      if token == open then
         n = n + 1 
      else
         if token == close then
            n = n - 1 
         end
      end
      if n >= 1 then
         if token ~= ";" then
            if nospace [ token ] then
               out ( token )
            else
               out ( token .. " " )
            end
         end
         token = gettoken ( source )
      end
   end
   table . remove ( exprstack )
   return token 
end

local expression_terminators = { [ ";" ] = 1 , -- [")"] = 1,
--	[","] = 1,
--	["]"] = 1,
--	["}"] = 1 *
}
local openclose = { [ "(" ] = ")" , [ "[" ] = "]" , [ "{" ] = "}" , }
function processexpression(source , token )
   table . insert ( exprstack , savepos ( source ) )
   while token do
      token = eatexpr ( source , token )
      if not token or expression_terminators [ token ] then
         break 
      end
      if nospace [ token ] then
         out ( token )
      else
         out ( token .. " " )
      end
      local close = openclose [ token ]
      if close then
         processblock ( source , token , close , 1 )
         out ( close )
      end
      token = gettoken ( source )
   end
   table . remove ( exprstack )
   return token 
end

function process_if(source , token , what )
   if token ~= "(" then
      emiterror ( "'(' expected" , source )
      return token 
   end
   outi ( )
   out ( what .. " " )
   token = processblock ( source , "(" , ")" , 1 )
   if what == "if" then
      out ( "then" )
   else
      out ( "do" )
   end
   outnl ( )
   outindent ( 1 )
   token = processstatement ( source , gettoken ( source ) , 1 )
   outindent ( - 1 )
   while what == "if" and token == "elseif" do
      token = gettoken ( source )
      if token ~= "(" then
         emiterror ( "'(' expected" , source )
         return token 
      end
      outi ( )
      out ( "elseif " )
      token = processblock ( source , "(" , ")" , 1 )
      out ( "then" )
      outnl ( )
      outindent ( 1 )
      token = processstatement ( source , gettoken ( source ) , 1 )
      outindent ( - 1 )
   end
   if what == "if" and token == "else" then
      outi ( )
      out ( token )
      outnl ( )
      outindent ( 1 )
      token = processstatement ( source , gettoken ( source ) , 1 )
      outindent ( - 1 )
   end
   outi ( )
   out ( "end" )
   outnl ( )
   return token 
end

keywords = { [ "if" ] = process_if , [ "while" ] = process_if , [ "for" ] = process_if , }
function processstatement(source , token , delimited )
   if keywords [ token ] then
      return keywords [ token ]( source , gettoken ( source ) , token )
   else
      if token == "{" then
         if not delimited then
            outi ( )
            out ( "do" )
            outnl ( )
            outindent ( 1 )
         end
         token = gettoken ( source )
         while token and token ~= "}" do
            token = processstatement ( source , token )
         end
         if not delimited then
            outindent ( - 1 )
            outi ( )
            out ( "end" )
            outnl ( )
         end
         token = gettoken ( source )
      else
         outi ( )
         token = processexpression ( source , token )
         outnl ( )
         if token and token ~= ";" and token ~= "}" then
            emiterror ( "warning ';' or '}' expected" , source )
            token = gettoken ( source )
         end
         if token == ";" then
            token = gettoken ( source )
         end
      end
   end
   return token 
end

function processsource(source )
   local token = gettoken ( source )
   while token do
      token = processstatement ( source , token )
   end
end

function loadmodule(name , ns )
   local mname = "mod_" .. name 
   local ons = getfenv ( )
   if ns then
      setfenv ( 1 , ns )
   end
   enter_namespace ( mname )
   local table = getfenv ( )
   local module , error = loadfile ( name .. ".lua" )
   if module then
      message ( format ( "Module '%s' loaded" , name ) )
      setfenv ( module , table )
      module ( )
      add_options ( table . options )
   else
      emiterror ( format ( "Could not load module '%s'" , name ) )
      message ( error )
      table = nil 
   end
   exit_namespace ( )
   setfenv ( 1 , ons )
   return table 
end

function jslua(f )
   has_error = nil 
   num_error = 0 
   resultString = { }
   local filename = f or "stdin" 
   message ( "Reading from " .. filename )
   local source = opensource ( f , filename )
   if not source then
      return "" 
   end
   message ( format ( "%d lines, %d tokens" , source . nline , source . ntoken ) )
   message ( "Processing " .. filename )
   processsource ( source )
   closesource ( source )
   if has_error then
      message ( format ( "%d error(s) while compiling" , num_error ) )
      return "" 
   else
      message ( format ( "no error while compiling" ) )
   end
   return colapse ( resultString )
end

function dofile(file )
   local source = jslua ( file )
   local module , error = loadstring ( source )
   source = nil 
   if module then
      module ( )
   else
      emiterror ( format ( "Could not load string" ) )
      message ( error )
   end
end

postprocess = { }
function do_postprocess()
   for _ , v in pairs ( postprocess ) do
      v ( )
   end
end

function add_postprocess(f )
   table . insert ( postprocess , f )
end

outcurindent = "" 
outindentstring = "   " 
outindentlevel = 0 
function out(s )
   table . insert ( resultString , s )
end

function outf(... )
   local s = format ( ... )
   out ( s )
end

function get_outcurindent()
   return outcurindent 
end

function outi()
   out ( outcurindent )
end

local line = 1 
function outnl()
   line = line + 1 
   out ( "\n" )
end

function outindent(l )
   outindentlevel = outindentlevel + l 
   outcurindent = string . rep ( outindentstring , outindentlevel )
end

function option_list(opt )
   for i , v in pairs ( opt ) do
      emiterror ( i .. " " .. v . help )
   end
end

function option_help()
   print ( "usage : jslua [options] [filenames]" )
   option_list ( options )
   os . exit ( )
end

function option_module()
   local name = option_getarg ( )
   loadmodule ( name )
end

local outhandle = io . stdout 
local compileonly 
function option_output()
   compileonly = 1 
   local fn = option_getarg ( )
   outhandle = io . open ( fn , "w" )
   if not outhandle then
      emiterror ( "Failed to open '" .. fn .. "' for writing." )
   else
      message ( "Opened '" .. fn .. "' for writing ..." )
   end
end

options = { [ "-o" ] = { call = option_output , help = "compile only, and output lua source code to specified file" } , [ "-c" ] = { call = function ()
   compileonly = 1 
end
, help = "compile only, and output lua source code on stdout" } , [ "-v" ] = { call = function ()
   verbose = 1 
end
, help = "turn verbose mode on" } , [ "-d" ] = { call = function ()
   dbgmode = 1 
end
, help = "turn debug mode on" } , [ "--module" ] = { call = option_module , help = "<modulename> load a module" } , [ "--help" ] = { call = option_help , help = "display this help message" } }
function add_options(table )
   for i , v in pairs ( table ) do
      if options [ i ] then
         emiterror ( format ( "Option '%s' overriden" , i ) )
      end
      options [ i ]= v 
   end
end

function option_getarg()
   local arg = option_args [ option_argind ]
   option_argind = option_argind + 1 
   return arg 
end

exit_namespace ( )
if standalone then
   local name = arg [ 0 ]
   if name then
      local i = 0 
      local j 
      while i ~= nil do
         j = i 
         i = string . find ( name , "[/\\]" , i + 1 )
      end
      if j then
         name = string . sub ( name , 0 , j )
         jslua . message ( format ( "Adding path '%s'" , name ) )
         jslua . add_path ( name )
      end
   end
   jslua . option_args = arg 
   jslua . option_argind = 1 
   local filename = { }
   while jslua . option_argind <= #jslua . option_args do
      local arg = jslua . option_getarg ( )
      if string . sub ( arg , 1 , 1 ) == "-" then
         local opt = jslua . options [ arg ]
         if opt then
            if opt . call then
               opt . call ( )
            end
            if opt . postcall then
               jslua . add_postprocess ( opt . postcall )
            end
         else
            jslua . emiterror ( format ( "Unknown option '%s'\n" , arg ) )
            jslua . option_help ( )
         end
      else
         table . insert ( filename , arg )
      end
   end
   local function doit(filename )
      if compileonly then
         outhandle : write ( jslua . jslua ( filename ) )
      else
         jslua . dofile ( filename )
      end
   end

   if not next ( filename ) then
      doit ( )
   else
      for _ , v in pairs ( filename ) do
         doit ( v )
      end
   end
   jslua . do_postprocess ( )
   if jslua . has_error then
      os . exit ( - 1 )
   end
end
