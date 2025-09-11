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
--	File system library:   lfs.chdir, lfs.currentdir, lfs.dir, lfs.mkdir, lfs.rmdir,
--                         lfs.copyfile, lfs.rename
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
		--"bzip2",
		--"minilzo",
		--"zip",              -- Depends: bzip2
		--"http",
		--"httpd",
		--"telnetd",
		--"ftpd",
		--"mongoose",
		--"ppp",
		--"mpg123",
		--"oggvorbis",
		--"adx",
		--"s3m",
		--"wave",
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
		--"bflash",
		--"openssl",
		--"bitcoin",
		--"quirc"
	},

	Initialize = function(self)

		os.execute("env USER Default");
		local path = os.getenv("PATH");
		local time = os.time();

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

		-- RTC validity check and fix.
		if time < 946684800 or time > 3471292800 then
			os.execute("rtc --set --unix 1735689600");
		end

		self:InstallingApps(path .. "/apps");
		self.initialized = true;
		OpenApp(os.getenv("APP"));

		local startup_cmd = os.getenv("STARTUP_CMD");
		if startup_cmd ~= nil and startup_cmd ~= "" then
			os.execute(startup_cmd);
		end
	end,

	InstallingApps = function(self, path)

		print("DS_PROCESS: Installing apps...\n");
		local name = nil;
		local list = {};

		for ent in lfs.dir(path) do
			if ent ~= nil and ent.name ~= ".." and ent.name ~= "." and ent.attr ~= 0 then
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
