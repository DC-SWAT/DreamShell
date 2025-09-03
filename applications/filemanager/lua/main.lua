-------------------------------------------
--                                       --
-- @name:    File Manager                --
-- @version: 0.8.0                       --
-- @author:  SWAT                        --
-- @url:     http://www.dc-swat.ru       --
--                                       --
-------------------------------------------


FileManager = {

	app = nil,
	font = nil,
	title = nil,
	prev_title = nil,
	modules = {},
	ffmpeg = {
		ext = {
			".mpg", ".m1v", ".m2v", ".sfd", ".pss", "mpeg", ".avi", 
			".mkv", ".m4v", ".flv", ".f4v", ".wmv", "m2ts", ".mpv", 
			".mp4", ".3gp", ".4xm", ".mov", ".mgp"
		}
	},
	audio = {
		ext = {
			".mp1", ".mp2", ".mp3", ".ogg", ".adx", ".s3m", ".wav",
			".raw", ".pcm", "dpcm"
		}
	},

	mgr = {

		top = {
			id = 0,
			widget = nil,
			focus = true,
			ent = {
				name = nil,
				size = -1,
				time = 0,
				attr = 1,
				index = -1
			}
		},

		bottom = {
			id = 1,
			widget = nil,
			focus = true,
			ent = {
				name = nil,
				size = -1,
				time = 0,
				attr = 1,
				index = -1
			}
		},

		dual = true,

		bg = {
		 	normal = nil,
		 	focus = nil
		},

		item = {
			normal = nil,
			selected = nil
		}
	},

	modal = {
		widget = nil,
		window = nil,
		label = nil,
		input = nil,
		progress = nil,
		body = nil,
		buttons = nil,
		ok = nil,
		cancel = nil,
		visible = true,
		func = nil,
		mode = nil,
		copy_cancelled = false
	},

	toolbar = {
		widget = nil
	}
}

function FileManager:ext_supported(tbl, ext)
	for i = 1, table.getn(tbl) do
		if tbl[i] == ext then
			return true;
		end
	end
	return false;
end

function FileManager:replace_spaces(str)
	return string.gsub(str, " ", "\\");
end


function FileManager:ShowModal(mode, text, func, inputText)

	if not self.modal.visible then
		GUI.ContainerSetEnabled(self.mgr.top.widget, 0);
		GUI.ContainerSetEnabled(self.mgr.bottom.widget, 0);
		GUI.ContainerSetEnabled(self.toolbar.widget, 0);
		GUI.ContainerAdd(self.app.body, self.modal.window);
	end

	self.modal.mode = mode;
	self.modal.func = func;

	if mode == "alert" then
		GUI.ContainerRemove(self.modal.body, self.modal.input);
		GUI.ContainerRemove(self.modal.body, self.modal.progress);
		GUI.WidgetSetEnabled(self.modal.buttons, 1);
		GUI.WidgetSetEnabled(self.modal.ok, 1);
		GUI.WidgetSetEnabled(self.modal.cancel, 0);

	elseif mode == "confirm" then
		GUI.ContainerRemove(self.modal.body, self.modal.input);
		GUI.ContainerRemove(self.modal.body, self.modal.progress);
		GUI.WidgetSetEnabled(self.modal.buttons, 1);
		GUI.WidgetSetEnabled(self.modal.ok, 1);
		GUI.WidgetSetEnabled(self.modal.cancel, 1);

	elseif mode == "prompt" then
		GUI.ContainerRemove(self.modal.body, self.modal.progress);
		GUI.ContainerAdd(self.modal.body, self.modal.input);
		GUI.TextEntrySetText(self.modal.input, inputText or "");
		GUI.WidgetSetEnabled(self.modal.buttons, 1);
		GUI.WidgetSetEnabled(self.modal.ok, 1);
		GUI.WidgetSetEnabled(self.modal.cancel, 1);

	elseif mode == "progress" then
		GUI.ContainerRemove(self.modal.body, self.modal.input);
		GUI.ContainerAdd(self.modal.body, self.modal.progress);
		GUI.ProgressBarSetPosition(self.modal.progress, 0);
		GUI.WidgetMarkChanged(self.modal.progress);
		GUI.WidgetSetEnabled(self.modal.buttons, 1);
		GUI.WidgetSetEnabled(self.modal.ok, 0);
		GUI.WidgetSetEnabled(self.modal.cancel, 1);
	end

	GUI.LabelSetText(self.modal.label, text);
	GUI.WidgetMarkChanged(self.modal.body);
    GUI.WidgetMarkChanged(self.app.body);

	self.modal.visible = true;
end


function FileManager:HideModal()

	if self.modal.visible then

		GUI.ContainerRemove(self.app.body, self.modal.window);
		GUI.WidgetMarkChanged(self.app.body);

		GUI.ContainerSetEnabled(self.mgr.top.widget, 1);
		GUI.ContainerSetEnabled(self.mgr.bottom.widget, 1);
		GUI.ContainerSetEnabled(self.toolbar.widget, 1);

		self.modal.visible = false;
		self.modal.func = nil;
		self.modal.mode = nil;
	end
end


function FileManager:ModalClick(s)

	if not s then
		if self.modal.mode == "progress" then
			self.modal.copy_cancelled = true;
		end
		return self:HideModal();
	end

	if s and self.modal.visible and self.modal.func then 
	
		GUI.WidgetSetEnabled(self.modal.ok, 0);
		
		if self.modal.func == "delete" then
		
			self:deletePath();
			
		elseif self.modal.func == "mount_iso" then
		
			self:toolbarMountISO();
			
		elseif self.modal.func == "copy" then
		    
			self:copyPath();
			
		elseif self.modal.func == "rename" then

			self:toolbarRename();
			
		elseif self.modal.func == "mkdir" then

			self:toolbarMkdir();

		elseif self.modal.func == "exec" then
		    
			self:openFile();
			
		elseif self.modal.func == "archive" then
		    
			self:toolbarArchive();
		else
			return self:ShowModal("alert", "Unknown command", nil);
		end 

		if self.modal.mode == "alert" then
			return;
		end
	end
	
	self:HideModal();
end


function FileManager:showConsole()
	ShowConsole();
end

function FileManager:hideConsole()
	Sleep(1000);
	HideConsole();
end

function FileManager:execConsole(cmd)
	self:showConsole();
	os.execute(cmd);
	self:hideConsole();
end


function FileManager:copyFile(src, dst, size)

	local initial_copied_size = self.modal.progress_copied_size;
	local buffer_size;

	if size < 5 * 1024 * 1024 then
		buffer_size = 65536;
	else
		buffer_size = 512 * 1024;
	end

    local function progress_callback(bytes_written)
        local current_progress = initial_copied_size + bytes_written;
        GUI.ProgressBarSetPosition(self.modal.progress, current_progress / self.modal.progress_total);
		GUI.WidgetMarkChanged(self.modal.progress);
		return not self.modal.copy_cancelled
    end

    local ok, err = lfs.copyfile(src, dst, progress_callback, buffer_size);
	self.copy_error = err;
    
	if not ok then
        return false;
    end

	self.modal.progress_copied_size = self.modal.progress_copied_size + size;
    
    return true;
end


function FileManager:getPathTotalSize(path)
    local total_size = 0;

    for file in lfs.dir(path) do
        if file.name ~= "." and file.name ~= ".." then
            local fullpath = path .. "/" .. file.name;

            if file.attr == 0 then
                total_size = total_size + file.size;
            else
                total_size = total_size + self:getPathTotalSize(fullpath);
            end
        end
    end
    return total_size;
end


function FileManager:copyPathRecursive(src, dst)
	
	if not lfs.mkdir(dst) then
		return false;
	end

    for file in lfs.dir(src) do
        if file.name ~= "." and file.name ~= ".." then

            local full_src_path = src .. "/" .. file.name;
			local full_dst_path = dst .. "/" .. file.name;

            if file.attr == 0 then
                if not self:copyFile(full_src_path, full_dst_path, file.size) then
					return false;
				end
            else
                if not self:copyPathRecursive(full_src_path, full_dst_path) then
					return false;
				end
            end
        end
    end
	return true;
end


function FileManager:copyPath()

	local f = self:getFile();
	local mgr = self:getUnfocusedManager();
	local to = GUI.FileManagerGetPath(mgr.widget);

	if to ~= "/" then
		to = to .. "/" .. f.name;
	else
		to = to .. f.name;
	end

	if f.attr ~= 0 then
		self:ShowModal("progress", "Calculating files size...", nil);
		Sleep(50);

		self.modal.progress_total = self:getPathTotalSize(f.file);
		self.modal.progress_copied_size = 0;
		self.modal.copy_cancelled = false;

		GUI.LabelSetText(self.modal.label, "Copying files, please wait...");
		GUI.WidgetMarkChanged(self.modal.label);
		Sleep(50);

		if not self:copyPathRecursive(f.file, to) then
			self:ShowModal("alert", "Error copying directory: " .. (self.copy_error or "unknown"));
			return;
		end
	else
		self:ShowModal("progress", "Copying files, please wait...", nil);
		Sleep(50);

		self.modal.progress_total = f.size;
		self.modal.progress_copied_size = 0;
		self.modal.copy_cancelled = false;

		if not self:copyFile(f.file, to, f.size) then
			self:ShowModal("alert", "Error copying file: " .. (self.copy_error or "unknown"));
			return;
		end
	end

	self:HideModal();
	GUI.FileManagerScan(mgr.widget);
end


function FileManager:toolbarCopy()

	local f = self:getFile();

	if f.name == nil then
		return;
	end

	local mgr = self:getUnfocusedManager();
	local to = GUI.FileManagerGetPath(mgr.widget);

	if not self.modal.visible then 
		return self:ShowModal("confirm", "Copy '" .. f.name .. "' to '" .. to .. "'?", "copy");
	else
		self:HideModal();
		self:copyPath();
	end
end


function FileManager:toolbarRename()

	local f = self:getFile();

	if f.name == nil then
		return;
	end

	if not self.modal.visible then 
		return self:ShowModal("prompt", "Enter a new name:", "rename", f.name);
	else
		self:HideModal();
	end

	local dst = f.path .. "/" .. GUI.TextEntryGetText(self.modal.input);
	local ok, err = lfs.rename(f.file, dst);

	if not ok then
		self:ShowModal("alert", "Error renaming item: " .. (err or "unknown"));
		return;
	end

	local mgr = self:getFocusedManager();
	GUI.FileManagerScan(mgr.widget);
end


function FileManager:toolbarMkdir()

	local mgr = self:getFocusedManager();
	local path = GUI.FileManagerGetPath(mgr.widget);

	if not self.modal.visible then 
		return self:ShowModal("prompt", "Enter a new directory name:", "mkdir", "NewFolder");
	else
		self:HideModal();
	end

	local dst = path .. "/" .. GUI.TextEntryGetText(self.modal.input);
	local ok, err = lfs.mkdir(dst);

	if not ok then
		self:ShowModal("alert", "Can't create directory: " .. (err or "unknown"));
	end
end


function FileManager:toolbarDelete()

	local f = self:getFile();
	local msg = 'Delete "' .. f.name .. '"?';

	if f.name == nil then
		return;
	end

	if f.attr ~= 0 then
		msg = 'Delete folder "'.. f.name ..'"?';
	end

	if not self.modal.visible then 
		return self:ShowModal("confirm", msg, "delete");
	else
		self:HideModal();
	end
end


function FileManager:countFiles(path)
    local count = 0;

    for file in lfs.dir(path) do
        if file.name ~= "." and file.name ~= ".." then

            local fullpath = path .. "/" .. file.name;
            count = count + 1;

            if file.attr ~= 0 then
                count = count + self:countFiles(fullpath);
            end
        end
    end

    return count;
end


function FileManager:deletePathRecursive(path)
    for file in lfs.dir(path) do
        if file.name ~= "." and file.name ~= ".." then

            local fullpath = path .. "/" .. file.name;

            if file.attr == 0 then
				local ok, err = os.remove(fullpath);

                if not ok then
					self.delete_error = err;
					return false;
				end
            else
                if not self:deletePathRecursive(fullpath) then
					return false;
				end
            end

			self.modal.progress_count = self.modal.progress_count + 1;
			GUI.ProgressBarSetPosition(self.modal.progress, self.modal.progress_count / self.modal.progress_total);
        end
    end
    
	local ok, err = lfs.rmdir(path);

	if not ok then
		self.delete_error = err;
		return false;
	end

    return true;
end


function FileManager:deletePath()

	local f = self:getFile();
	self.delete_error = nil;

	if f.attr ~= 0 then

		self:ShowModal("progress", "Calculating files...", nil);
		Sleep(50);

		self.modal.progress_total = self:countFiles(f.file);
		self.modal.progress_count = 0;

		GUI.LabelSetText(self.modal.label, "Deleting files, please wait...");
		GUI.WidgetMarkChanged(self.modal.label);
		Sleep(50);

		if not self:deletePathRecursive(f.file) then
			self:ShowModal("alert", "Error deleting: " .. (self.delete_error or "unknown"));
			return;
		end

	else
		local ok, err = os.remove(f.file);
		if not ok then
			self:ShowModal("alert", "Error deleting: " .. (err or "unknown"));
			return;
		end
	end

	self:HideModal();

	local mgr = self:getFocusedManager();
	GUI.FileManagerScan(mgr.widget);

	mgr.ent = {name = nil, size = 0, time = 0, attr = 0, index = -1};
	local umgr = self:getUnfocusedManager();

	if umgr and umgr.ent and umgr.ent.index > -1 then
		local bt = GUI.FileManagerGetItem(umgr.widget, umgr.ent.index);

		if bt then
			GUI.ButtonSetNormalImage(bt, self.mgr.item.normal);
		end

		umgr.ent = {name = nil, size = 0, time = 0, attr = 0, index = -1};
	end
end


function FileManager:toolbarArchive()

	local f = self:getFile();

	if f.name == nil then
		return;
	end

	local ext = string.lower(string.sub(f.name, -4));
	local file = self:replace_spaces(f.file);
	local name = f.name;

	local mgr = self:getUnfocusedManager();
	local dst = GUI.FileManagerGetPath(mgr.widget);

	local msg = "Uknown";
	local cmd = "";

	if ext == ".gz" then

	    msg = "Extract ";
	    cmd = "gzip -d " .. file .. " " .. dst .. string.sub(name, 1, -4);

	elseif ext == "bz2" then

		if not DS.GetCmdByName("bzip2") then
			 if not self:loadModule("bzip2") then
				self:showConsole();
				return;
			 end
		end

	    msg = "Extract ";
	    cmd = "bzip2 -d " .. file .. " " .. dst .. string.sub(name, 1, -5);

	elseif ext == "zip" then

		if not DS.GetCmdByName("zip") then
			 if not self:loadModule("zip") then
				self:showConsole();
				return;
			 end
		end

	    msg = "Extract ";
	    cmd = "unzip -e -o " .. file .. " -d " .. dst;

	else

		msg = "Compress ";
		cmd = "gzip -9 " .. file .. " " .. dst .. name .. ".gz";

	end

	if not self.modal.visible then 
		return self:ShowModal("confirm", msg .. '"' .. name .. '"?', "archive");
	else
		self:HideModal();
	end

	self:execConsole(cmd)
	GUI.FileManagerScan(mgr.widget);
end


function FileManager:toolbarModeSwitch()

	if self.mgr.dual then
		GUI.ContainerRemove(self.app.body, self.mgr.bottom.widget);
		GUI.FileManagerResize(self.mgr.top.widget, 610, 439);
		GUI.FileManagerSetScrollbar(self.mgr.top.widget, nil, self:getResource("sb-back-big", DS.LIST_ITEM_GUI_SURFACE));
		self.mgr.dual = false;
	else
		GUI.FileManagerResize(self.mgr.top.widget, 610, 217);
		GUI.FileManagerSetScrollbar(self.mgr.top.widget, nil, self:getResource("sb-back", DS.LIST_ITEM_GUI_SURFACE));
		GUI.ContainerAdd(self.app.body, self.mgr.bottom.widget);
		GUI.WidgetMarkChanged(self.app.body);
		self.mgr.dual = true;
	end
end


function FileManager:toolbarMountISO()

	local f = self:getFile();

	if f.name == nil then
		return;
	end

	if not self.modal.visible then 
		return self:ShowModal("confirm", 'Mount selected ISO as VFS?', "mount_iso");
	else
		self:HideModal();
	end

	if not DS.GetCmdByName("isofs") then
		 if not self:loadModule("minilzo") or not self:loadModule("isofs") then
			self:showConsole();
			return;
		 end
	end

	if os.execute("isofs -m -f " .. self:replace_spaces(f.file) .. " -d /iso") ~= DS.CMD_OK then
		self:showConsole();
	end
end




function FileManager:loadModule(name)

	local file = os.getenv("PATH") .. "/modules/" .. name .. ".klf";
	local m = OpenModule(file);

	if m ~= nil then
		table.insert(self.modules, m);
		return true;
	end

	return false;
end


function FileManager:unloadModules()

    if table.getn(self.modules) > 0 then

		for i = 1, table.getn(self.modules) do

			if self.modules[i] ~= nil then
				CloseModule(self.modules[i]);
			end

			table.remove(self.modules, i);
		end
	end
end


function FileManager:openApp(name, args, unload_fm)
	self:Shutdown();
	CloseApp(self.app.name, unload_fm);
	OpenApp(name, args);
end


function FileManager:openFile()

	local f = self:getFile();

	if f.name == nil then
		return;
	end

	local ext = string.lower(string.sub(f.name, -4));
	local file = self:replace_spaces(f.file);
	local name = f.name;

	if ext == ".bin" or ext == ".elf" then

		if not self.modal.visible then
			return self:ShowModal("confirm", 'Execute "' .. name .. '" ?', "exec");
		else
			self:HideModal();
		end

		local flag = "-b";

  		if ext == ".elf" then
  		   flag = "-e";
  		end

		self:execConsole("exec "..flag.." -f " .. file);

	elseif ext == ".txt" then

		if not self.modal.visible then
			return self:ShowModal("confirm", 'Cat file "' .. name .. '" to console?', "exec");
		else
			self:HideModal();
		end

		self:showConsole();
		os.execute("cat " .. file);

	elseif ext == ".klf" then

		if not self.modal.visible then
			return self:ShowModal("confirm", 'Load module "' .. name .. '" ?', "exec");
		else
			self:HideModal();
		end

		self:execConsole("module -o -f " .. file)

	elseif ext == ".lua" then

		if not self.modal.visible then
			return self:ShowModal("confirm", 'Run lua script "' .. name .. '" ?', "exec");
		else
			self:HideModal();
		end

		self:execConsole("lua " .. file)
	
	elseif ext == ".dsc" then

		if not self.modal.visible then
			return self:ShowModal("confirm", 'Run cmd script "' .. name .. '" ?', "exec");
		else
			self:HideModal();
		end

		self:execConsole("dsc " .. file)

	elseif ext == ".xml" then

		if not self.modal.visible then
			return self:ShowModal("confirm", 'Add application "' .. name .. '" ?', "exec");
		else
			self:HideModal();
		end

		self:execConsole("app -a -f " .. file)

	elseif ext == ".dsr" or ext == ".img" then

		if not self.modal.visible then
			return self:ShowModal("confirm", 'Mount romdisk image "' .. name .. '" ?', "exec");
		else
			self:HideModal();
		end

		self:execConsole("romdisk -m " .. file)

	elseif self:ext_supported(self.audio.ext, ext) then

		local mod = string.sub(ext, -3);

		if mod == "raw" or mod == "wav" or mod == "pcm" then
			mod = "wave"
		elseif mod == "ogg" then
			mod = "oggvorbis"
		elseif mod == "mp1" or mod == "mp2" or mod == "mp3" then
			mod = "mpg123"
		end

		if not self.modal.visible then
			return self:ShowModal("confirm", 'Play file "' .. name .. '" ?', "exec");
		else
			self:HideModal();
		end

		if not DS.GetCmdByName(mod) then

			 if not self:loadModule(mod) then
			 	self:showConsole();
			 	return file;
			 end
		end

		if os.execute(mod .. " -p -f " .. file) ~= DS.CMD_OK then
			self:showConsole();
		end

	elseif self:ext_supported(self.ffmpeg.ext, ext) then

		if not self.modal.visible then
			return self:ShowModal("confirm", 'Play file "' .. name .. '" ?', "exec");
		else
			self:HideModal();
		end

		if not DS.GetCmdByName("ffplay") then
		
			 if not self:loadModule("bzip2") 
				 or not self:loadModule("mpg123") 
				 or not self:loadModule("oggvorbis") 
				 or not self:loadModule("ffmpeg") then
			 	self:showConsole();
			 	return file;
			 end
		end

		if os.execute("ffplay -p -f " .. file) ~= DS.CMD_OK then
			self:showConsole();
		end

	elseif ext == ".opk" then

		if not self.modal.visible then
			return self:ShowModal("confirm", 'Install "' .. name .. '" package?', "exec");
		else
			self:HideModal();
		end

		if not DS.GetCmdByName("opkg") then
		
			 if not self:loadModule("minilzo") or not self:loadModule("opkg") then
			 	self:showConsole();
			 	return file;
			 end
		end

		self:execConsole("opkg -i -f " .. file);

	else

		local app = DS.GetAppByExtension(ext);

		if app ~= nil then

			if not self.modal.visible then
				return self:ShowModal("confirm", 'Open file in ' .. app.name .. ' app?', "exec");
			else
				self:HideModal();
			end

			self:openApp(app.name, f.file, 0); -- Unload filemanager?
			return file;
		end

		local mgr = self:getFocusedManager();
		local bt = GUI.FileManagerGetItem(mgr.widget, mgr.ent.index);
		GUI.ButtonSetNormalImage(bt, self.mgr.item.normal);
		mgr.ent = {name = nil, size = 0, time = 0, attr = 0, index = -1};
	end

	return file;
end


function FileManager:focusManager(mgr)

	mgr.focus = true;
	GUI.PanelSetBackground(mgr.widget, self.mgr.bg.focus);

	if mgr.id == self.mgr.top.id then
		self.mgr.bottom.focus = false;
		GUI.PanelSetBackground(self.mgr.bottom.widget, self.mgr.bg.normal);
	elseif mgr.id == self.mgr.bottom.id then
		self.mgr.top.focus = false;
		GUI.PanelSetBackground(self.mgr.top.widget, self.mgr.bg.normal);
	end
end


function FileManager:getFocusedManager()
	if self.mgr.top.focus then
	   return self.mgr.top;
	elseif self.mgr.bottom.focus then
	   return self.mgr.bottom;
    end
end

function FileManager:getUnfocusedManager()
	if not self.mgr.top.focus then
	   return self.mgr.top;
	elseif not self.mgr.bottom.focus then
	   return self.mgr.bottom;
    end
end


function FileManager:getFile() 

	local mgr = self:getFocusedManager();
	local path = GUI.FileManagerGetPath(mgr.widget);
	local file = path .. "/";

	if mgr.ent.name ~= nil then
		file = file .. mgr.ent.name
	end

	return {
		file = file,
		name = mgr.ent.name,
		path = path,
		attr = mgr.ent.attr,
		size = mgr.ent.size
	};
end


function FileManagerItemClickTop(ent)
	FileManager:ItemClick(ent, FileManager.mgr.top);
end

function FileManagerItemContextClickTop(ent)
	FileManager:ItemContextClick(ent, FileManager.mgr.top);
end

function FileManagerItemClickBottom(ent)
	FileManager:ItemClick(ent, FileManager.mgr.bottom);
end

function FileManagerItemContextClickBottom(ent)
	FileManager:ItemContextClick(ent, FileManager.mgr.bottom);
end


function FileManager:ItemClick(ent, mgr)

	self:focusManager(mgr);
	
	local umgr = self:getUnfocusedManager();

	if umgr and umgr.ent and umgr.ent.index > -1 then
		local bt = GUI.FileManagerGetItem(umgr.widget, umgr.ent.index);
		if bt then
			GUI.ButtonSetNormalImage(bt, self.mgr.item.normal);
		end
		umgr.ent = {name = nil, size = 0, time = 0, attr = 0, index = -1};
	end

	if ent.attr == 0  then

		if not mgr.ent or (mgr.ent.index ~= ent.index and mgr.ent.name ~= ent.name) then

			local bt;

			if mgr.ent.index > -1 then
				bt = GUI.FileManagerGetItem(mgr.widget, mgr.ent.index);
				GUI.ButtonSetNormalImage(bt, self.mgr.item.normal);
			end

			bt = GUI.FileManagerGetItem(mgr.widget, ent.index);
			GUI.ButtonSetNormalImage(bt, self.mgr.item.selected);
			mgr.ent = ent;

		else
			self:openFile();
		end
	else

		GUI.FileManagerChangeDir(mgr.widget, ent.name, ent.size);
		mgr.ent = {name = nil, size = 0, time = 0, attr = 0, index = -1};

		local d = GUI.FileManagerGetPath(mgr.widget);
		if d ~= "/" then
			self:tooltip(d);
		else 
			self:tooltip(nil);
		end
	end
end

function FileManager:ItemContextClick(ent, mgr)

	self:focusManager(mgr);
	
	local umgr = self:getUnfocusedManager();

	if umgr and umgr.ent and umgr.ent.index > -1 then
		local bt = GUI.FileManagerGetItem(umgr.widget, umgr.ent.index);
		if bt then
			GUI.ButtonSetNormalImage(bt, self.mgr.item.normal);
		end
		umgr.ent = {name = nil, size = 0, time = 0, attr = 0, index = -1};
	end

	if not mgr.ent or (mgr.ent.index ~= ent.index and mgr.ent.name ~= ent.name) then
	
		local bt;

		if mgr.ent.index > -1 then
			bt = GUI.FileManagerGetItem(mgr.widget, mgr.ent.index);
			GUI.ButtonSetNormalImage(bt, self.mgr.item.normal);
		end
		
		bt = GUI.FileManagerGetItem(mgr.widget, ent.index);
		GUI.ButtonSetNormalImage(bt, self.mgr.item.selected);
		mgr.ent = ent;
		
	end
end


function FileManagerItemMouseover(ent)
	if ent.attr == 0 then
		FileManager.prev_title = string.format("File size: %d KB", (ent.size / 1024));
		FileManager:tooltip(FileManager.prev_title);
	else
		if FileManager.prev_title ~= nil then
			FileManager.prev_title = nil;
			FileManager:tooltip(nil);
		end
	end
end


function FileManager:showError(str)
	print(str .. "\n");
	self:ShowModal("alert", str, nil);
	return false;
end


function FileManager:tooltip(msg)

	if msg then
		GUI.LabelSetText(self.title, msg);
	else

		local mgr = self:getFocusedManager();
		local path = GUI.FileManagerGetPath(mgr.widget);

		if path == "/" then
			GUI.LabelSetText(self.title, self.app.name .. " v" .. self.app.ver);
		else
			GUI.LabelSetText(self.title, path);
		end
	end
end


function FileManager:getResource(name, type)

	local r = DS.listGetItemByName(self.app.resources, name);

	if r ~= nil then

	    if type == DS.LIST_ITEM_GUI_FONT then
	    	return GUI.AnyToFont(r.data);
		elseif type == DS.LIST_ITEM_GUI_SURFACE then
		    return GUI.AnyToSurface(r.data);
		else
            self:showError("FileManager: Uknown resource type - " .. type);
			return nil;
		end
	end

	self:showError("FileManager: Can't find resource - " .. name);
	return nil;
end


function FileManager:getElement(name)

	local r = DS.listGetItemByName(self.app.elements, name);

	if r ~= nil then
		return GUI.AnyToWidget(r.data);
	end

	self:showError("FileManager: Can't find element - " .. name);
	return nil;
end


function FileManager:Initialize()

	if self.app == nil then

		self.app = DS.GetAppById(THIS_APP_ID);

		if self.app ~= nil then

	        self.font = self:getResource("arial", DS.LIST_ITEM_GUI_FONT);

			if not self.font then
				return false;
			end

			self.mgr.bg.normal = self:getResource("white-bg", DS.LIST_ITEM_GUI_SURFACE);

			if not self.mgr.bg.normal then
				return false;
			end

			self.mgr.bg.focus = self:getResource("blue-bg", DS.LIST_ITEM_GUI_SURFACE);

			if not self.mgr.bg.focus then
				return false;
			end

			self.title = self:getElement("title");

			if not self.title then
				return false;
			end

			self.mgr.top.widget = self:getElement("filemgr-top");

			if not self.mgr.top.widget then
				return false;
			end

			self.mgr.bottom.widget = self:getElement("filemgr-bottom");

			if not self.mgr.bottom.widget then
				return false;
			end

			self.mgr.item.normal = self:getResource("item-normal", DS.LIST_ITEM_GUI_SURFACE);

			if not self.mgr.item.normal then
				return false;
			end

			self.mgr.item.selected = self:getResource("item-selected", DS.LIST_ITEM_GUI_SURFACE);

			if not self.mgr.item.selected then
				return false;
			end

			self.modal.window = self:getElement("modal-win");

			if not self.modal.window then
				return false;
			end

			self.modal.widget = self:getElement("modal-content");

			if not self.modal.widget then
				return false;
			end

			self.modal.label = self:getElement("modal-label");

			if not self.modal.label then
				return false;
			end

			self.modal.input = self:getElement("modal-input");

			if not self.modal.input then
				return false;
			end

			self.modal.ok = self:getElement("modal-ok");

			if not self.modal.ok then
				return false;
			end

			self.modal.cancel = self:getElement("modal-cancel");

			if not self.modal.cancel then
				return false;
			end

			self.modal.buttons = self:getElement("modal-buttons");

			if not self.modal.buttons then
				return false;
			end

			self.modal.body = self:getElement("modal-body-panel");

			if not self.modal.body then
				return false;
			end

			self.modal.progress = self:getElement("modal-progress");

			if not self.modal.progress then
				return false;
			end

			self.toolbar.widget = self:getElement("toolbar-panel");

			if not self.toolbar.widget then
				return false;
			end

			self:HideModal();
			self:focusManager(self.mgr.top);
			self:toolbarModeSwitch();
			self:tooltip(nil);
		end
	end
end


function FileManager:Shutdown()
	self:unloadModules();
end
