-----------------------------------------
--                                     --
-- @name:     Startup script           --
-- @author:   SWAT                   	--
-- @url:      http://www.dc-swat.ru    --
--                                     --
-----------------------------------------
--
-- Internal DreamShell lua functions:
-- 
--	OpenModule             Open module file and return ID
--	CloseModule            Close module by ID
--	GetModuleByName        Get module ID by module NAME
--
--	AddApp                 Add app by XML file, return app NAME
--	OpenApp                Open app by NAME (second argument for args)
--	CloseApp               Close app by NAME (second argument for change unload flag)
--
--	ShowConsole
--	HideConsole
--	SetDebugIO             Set the debug output (scif, dclsocket, fb, ds, sd). By default is ds.
--	Sleep                  Sleep in current thread (in ms)
--	MapleAttached          Check for attached maple device
--
--	Bit library:           bit.or, bit.and, bit.not, bit.xor
--	File system library:   lfs.chdir, lfs.currentdir, lfs.dir, lfs.mkdir, lfs.rmdir
--	
------------------------------------------

local DreamShell = {

	initialized = false,
	
	modules = {
		--"tolua",
		--"tolua_2plus",
		--"luaDS",            -- Depends: tolua
		--"luaKOS",           -- Depends: tolua
		--"luaSDL",           -- Depends: tolua
		--"luaGUI",           -- Depends: tolua
		--"luaMXML",          -- Depends: tolua
		--"luaSTD",           -- Depends: tolua
		--"sqlite3",
		--"luaSQL",           -- Depends: sqlite3
		--"luaSocket",
		--"luaTask",
		--"angelscript",
		--"bzip2",
		--"minilzo",
		--"zip",              -- Depends: bzip2
		--"http",
		--"httpd",
		--"telnetd",
		--"mongoose",
		--"ppp",
		--"mpg123",
		--"oggvorbis",
		--"adx",
		--"s3m",
		--"xvid",
		--"SDL_mixer",        -- Depends: oggvorbis
		--"ffmpeg",           -- Depends: oggvorbis, mpg123, bzip2
		--"opengl",
		--"isofs",            -- Depends: minilzo
		--"isoldr",           -- Depends: isofs
		--"SDL_net",
		--"opkg",             -- Depends: minilzo
		--"aicaos",
		--"gumbo",
		--"ini",
		--"aicaos",
		--"bflash"
	},

	Initialize = function(self)

		os.execute("env USER Default");
		local path = os.getenv("PATH");

		print(os.getenv("HOST") .. " " .. os.getenv("VERSION") .. "\n");
		print(os.getenv("ARCH") .. ": " .. os.getenv("BOARD_ID") .. "\n");
		print("Date: " .. os.date() .. "\n");
		print("Base path: " .. path .. "\n");
		print("User: " .. os.getenv("USER") .. "\n");
		
		local emu = os.getenv("EMU");
		
		if emu ~= nil then
			print("Emulator: " .. emu .. "\n");
		end
		
		print("\n");
		
		if not MapleAttached("Keyboard") then
			table.insert(self.modules, "vkb");
		end

		table.foreach(self.modules, function(k, name)  
			print("DS_PROCESS: Loading module " .. name .. "...\n");
			if not OpenModule(path .. "/modules/" .. name .. ".klf") then
				print("DS_ERROR: Can't load module " .. path .. "/modules/" .. name .. ".klf\n");
			end
		end);
		
		self:CheckUpdates(path);
		self:InstallingApps(path .. "/apps");
		OpenApp(os.getenv("APP"));
		self.initialized = true;
	end,
	
	CheckUpdates = function(self, path)
	
		print("DS_PROCESS: Checking for updates on " .. path .. " ...\n");
	
		local f = io.open(path.. "/update/version.dat");
		local v = os.getenv("VERSION");
		
		local ins = false;
		local update = "";
		local uf = nil;
		
		for ver in f:lines() do 

			if ins then
			
				update = path .. "/update/pkg/" .. ver .. ".opk";
				uf = io.open(update);
				
				if uf ~= nil then
				
					uf:close();
					ShowConsole();
					print("DS_PROCESS: Installing update " .. update .. "\n");
					
					if not GetModuleByName("opkg") then
						if not OpenModule(path .. "/modules/minilzo.klf") or 
							not OpenModule(path .. "/modules/opkg.klf") then
							print("DS_ERROR: Can't open opkg module\n");
							f:close();
							HideConsole();
							return;
						end
					end
					
					os.execute("opkg -i -f " .. update);
					HideConsole();
				end
			end
			
			if ver == v then 
				ins = true;
			end
		end
		
		f:close();
	end,

	InstallingApps = function(self, path)

		print("DS_PROCESS: Installing apps...\n");
		local name = nil;
		local list = {};

		for ent in lfs.dir(path) do
			if ent ~= nil and ent.name ~= ".." and ent.name ~= "." and ent.name ~= ".svn" and ent.attr ~= 0 then
				table.insert(list, ent.name);
			end
		end

		table.sort(list, function(a, b) return a > b end);
		
		for index, directory in ipairs(list) do

			name = AddApp(path .. "/" .. directory .. "/app.xml");
			
			if not name then
				print("DS_ERROR: " .. directory .. "\n");
			else
				print("DS_OK: " .. name .. "\n");
			end
		end
		
		return true;
	end
};

if not DreamShell.initialized then
	DreamShell:Initialize();
end
