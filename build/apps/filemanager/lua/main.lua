-------------------------------------------
--                                       --
-- @name:    File Manager                --
-- @version: 0.6.3                       --
-- @author:  SWAT                        --
-- @url:     http://www.dc-swat.ru       --
--                                       --
-------------------------------------------


--if not FileManager then


FileManager = {

	app = nil,
	font = nil,
	title = nil,
	modules = {},
	ffmpeg = {
		ext = {
			".mpg", ".m1v", ".m2v", ".sfd", ".pss", "mpeg", ".avi", 
			".mkv", ".m4v", ".flv", ".f4v", ".wmv", "m2ts", ".mpv", 
			".mp4", ".3gp", ".4xm", ".mov", ".mgp"
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
		label = nil,
		input = nil,
		ok = nil,
		cancel = nil,
		visible = true,
		func = nil,
		mode = "prompt"
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


function FileManager:ShowModal(mode, label, func, input)

	
	if self.modal.visible then 
		self:HideModal(); 
	end
	
	--print("FileManager: " .. label .. "\n");
	
	if mode == "alert" then
	
		GUI.LabelSetText(self.modal.label, label);
		
		if self.modal.mode ~= "alert" then
			GUI.WidgetSetEnabled(self.modal.cancel, 0);
			GUI.WidgetSetPosition(self.modal.ok, 145, 116);
		end
		
		if self.modal.mode == "prompt" then
			GUI.ContainerRemove(self.modal.widget, self.modal.input);
		end	
	
	elseif mode == "confirm" then
	
		if self.modal.mode == "prompt" then
			GUI.ContainerRemove(self.modal.widget, self.modal.input);
		end
		
		GUI.LabelSetText(self.modal.label, label);

		GUI.WidgetSetPosition(self.modal.ok, 110, 116);
		GUI.WidgetSetEnabled(self.modal.cancel, 1);
		
	
	elseif mode == "prompt" then
	
		if self.modal.mode ~= "prompt" then
			GUI.ContainerAdd(self.modal.widget, self.modal.input);
		end
		
		GUI.LabelSetText(self.modal.label, label);
		GUI.TextEntrySetText(self.modal.input, input);
		
		if self.modal.mode == "alert" then
			GUI.WidgetSetPosition(self.modal.ok, 110, 116);
			GUI.WidgetSetEnabled(self.modal.cancel, 1);
		end
	
	else
		return false;
	end
	
	GUI.ContainerSetEnabled(self.mgr.top.widget, 0);
	GUI.ContainerSetEnabled(self.mgr.bottom.widget, 0);
	GUI.ContainerAdd(self.app.body, self.modal.widget);
	GUI.WidgetMarkChanged(self.modal.widget);
	
	self.modal.func = func;
	self.modal.visible = true;
	self.modal.mode = mode;
	
	return true;
end


function FileManager:HideModal()

	if self.modal.visible then
	
		GUI.ContainerRemove(self.app.body, self.modal.widget);
		
		GUI.ContainerSetEnabled(self.mgr.top.widget, 1);
		GUI.ContainerSetEnabled(self.mgr.bottom.widget, 1);
	
		self.modal.visible = false;
		self.modal.func = nil;
	end
end


function FileManager:ModalClick(s)

	if s and self.modal.visible and self.modal.func then 
		
		if self.modal.func == "delete" then
		
			self:toolbarDelete();
			
		elseif self.modal.func == "mount_iso" then
		
			self:toolbarMountISO();
			
		elseif self.modal.func == "copy" then
		    
			self:toolbarCopy();
			
		elseif self.modal.func == "rename" then

			self:toolbarRename();
			
		elseif self.modal.func == "exec" then
		    
			self:openFile();
			
		elseif self.modal.func == "archive" then
		    
			self:toolbarArchive();
		else
			return self:ShowModal("alert", "Uknown command", nil);
		end 
	end
	
	self:HideModal();
end



function FileManager:toolbarCopy()

	local f = self:getFile();
	
	local mgr = self:getUnfocusedManager();
	local to = GUI.FileManagerGetPath(mgr.widget);
	
	if not self.modal.visible then 
		return self:ShowModal("confirm", "Copy '" .. f.name .. "' to '" .. to .. "'?", "copy");
	else
		self:HideModal();
	end

	if to ~= "/" then
		to = to .. "/" .. f.name;
	else
		to = to .. f.name;
	end
	
	if os.execute("cp " .. f.file .. " " .. to) ~= DS.CMD_OK then
		return os.execute("console --show");
	end
	
	GUI.FileManagerScan(mgr.widget);
	
end



function FileManager:toolbarRename()

	local f = self:getFile();

	if not self.modal.visible then 
		return self:ShowModal("prompt", "Enter new name:", "rename", f.name);
	else
		self:HideModal();
	end
	
	local dst = f.path .. "/" .. GUI.TextEntryGetText(self.modal.input);
	
	if os.execute("rename " .. f.file .. " " .. dst) ~= DS.CMD_OK then
		return os.execute("console --show");
	end
	
	local mgr = self:getFocusedManager();
	GUI.FileManagerScan(mgr.widget);
end




function FileManager:toolbarDelete()

	local f = self:getFile();
	
	if not self.modal.visible then 
		return self:ShowModal("confirm", 'Delete "' .. f.name .. '"?', "delete");
	else
		self:HideModal();
	end
	
	if os.execute("rm " .. f.file) ~= DS.CMD_OK then
		os.execute("console --show");
		return false;
	end
	
	local mgr = self:getFocusedManager();
	GUI.FileManagerScan(mgr.widget);
end



function FileManager:toolbarArchive()

	local f = self:getFile();
	local ext = string.lower(string.sub(f.name, -4));
	local file = f.file;
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
				os.execute("console --show");
				return;
			 end
		end
	
	    msg = "Extract ";
	    cmd = "bzip2 -d " .. file .. " " .. dst .. string.sub(name, 1, -5);
	
	elseif ext == "zip" then
	
		if not DS.GetCmdByName("zip") then
			 if not self:loadModule("zip") then
				os.execute("console --show");
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
	
	os.execute("console --show");
	if os.execute(cmd) ~= DS.CMD_OK then
		--return os.execute("console --show");
	end

	GUI.FileManagerScan(mgr.widget);
	os.execute("sleep 1000\nconsole --hide");
end



function FileManager:toolbarModeSwitch()

	if self.mgr.dual then
		GUI.ContainerRemove(self.app.body, self.mgr.bottom.widget);
		GUI.FileManagerResize(self.mgr.top.widget, 610, 445);
		GUI.FileManagerSetScrollbar(self.mgr.top.widget, nil, self:getResource("sb-back-big", DS.LIST_ITEM_GUI_SURFACE));
		self.mgr.dual = false;
	else
		GUI.FileManagerResize(self.mgr.top.widget, 610, 220);
		GUI.FileManagerSetScrollbar(self.mgr.top.widget, nil, self:getResource("sb-back", DS.LIST_ITEM_GUI_SURFACE));
		GUI.ContainerAdd(self.app.body, self.mgr.bottom.widget);
		GUI.WidgetMarkChanged(self.app.body);
		self.mgr.dual = true;
	end
end


function FileManager:toolbarMountISO()

	local f = self:getFile();
	
	if not self.modal.visible then 
		return self:ShowModal("confirm", 'Mount selected ISO as VFS?', "mount_iso");
	else
		self:HideModal();
	end
	
	if not DS.GetCmdByName("isofs") then
		 if not self:loadModule("minilzo") or not self:loadModule("isofs") then
			os.execute("console --show");
			return;
		 end
	end
	
	if os.execute("isofs -m -f " .. f.file .. " -d /iso") ~= DS.CMD_OK then
		os.execute("console --show");
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
	self:Shutdown(); -- Unload user modules?
	CloseApp(self.app.name, unload_fm);
	OpenApp(name, args);
end


function FileManager:openFile()

	local f = self:getFile();
	local ext = string.lower(string.sub(f.name, -4));
	local file = f.file;
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

		os.execute("console --show\nsleep 1100");
		if os.execute("exec "..flag.." -f " .. file) ~= DS.CMD_OK then
			--os.execute("console --show");
		end
		os.execute("sleep 1000\nconsole --hide");
	
	elseif ext == ".txt" then
	
		if not self.modal.visible then 
			return self:ShowModal("confirm", 'Cat file "' .. name .. '" to console?', "exec");
		else
			self:HideModal();
		end
		
		os.execute("console --show\ncat " .. file);
	
	elseif ext == ".klf" then
	
		if not self.modal.visible then 
			return self:ShowModal("confirm", 'Load module "' .. name .. '" ?', "exec");
		else
			self:HideModal();
		end
		
		--if DS.GetModuleByFileName(file) then return file end
		
		os.execute("console --show");
		if os.execute("module -o -f " .. file) ~= DS.CMD_OK then
			--os.execute("console --show");
		end

		os.execute("sleep 1000\nconsole --hide");
		
	elseif ext == ".lua" then
	
		if not self.modal.visible then 
			return self:ShowModal("confirm", 'Run lua script "' .. name .. '" ?', "exec");
		else
			self:HideModal();
		end
		
		os.execute("console --show\nlua " .. file)
		os.execute("sleep 1000\nconsole --hide");
	
	elseif ext == ".dsc" then
	
		if not self.modal.visible then 
			return self:ShowModal("confirm", 'Run cmd script "' .. name .. '" ?', "exec");
		else
			self:HideModal();
		end
		
		os.execute("console --show");
		if os.execute("dsc " .. file) ~= DS.CMD_OK then
			--os.execute("console --show");
		end
		os.execute("sleep 1000\nconsole --hide");
		
	elseif ext == ".xml" then
	
		if not self.modal.visible then 
			return self:ShowModal("confirm", 'Add application "' .. name .. '" ?', "exec");
		else
			self:HideModal();
		end
		
		os.execute("console --show");
		if os.execute("app -a -f " .. file) ~= DS.CMD_OK then
			--os.execute("console --show");
		end
		os.execute("sleep 1000\nconsole --hide");
		
	elseif ext == ".dsr" or ext == ".img" then
	
		if not self.modal.visible then 
			return self:ShowModal("confirm", 'Mount romdisk image "' .. name .. '" ?', "exec");
		else
			self:HideModal();
		end
		
		os.execute("console --show");
		if os.execute("romdisk -m " .. file) ~= DS.CMD_OK then
			--os.execute("console --show");
		end

		os.execute("sleep 1000\nconsole --hide");
		
	elseif ext == ".mp1" or ext == ".mp2" or ext == ".mp3" then
		
		if not self.modal.visible then 
			return self:ShowModal("confirm", 'Play file "' .. name .. '" ?', "exec");
		else
			self:HideModal();
		end
		
		if not DS.GetCmdByName("mp3") then
		
			 if not self:loadModule("mpg123") then
			 	os.execute("console --show");
			 	return file; 
			 end
		end
		
		if os.execute("mpg123 -p -f " .. file) ~= DS.CMD_OK then
			os.execute("console --show");
		end
		
	elseif ext == ".ogg" then
	
		if not self.modal.visible then 
			return self:ShowModal("confirm", 'Play file "' .. name .. '" ?', "exec");
		else
			self:HideModal();
		end

		if not DS.GetCmdByName("oggvorbis") then
		
			 if not self:loadModule("oggvorbis") then
			 	os.execute("console --show");
			 	return file; 
			 end
		end
		
		if os.execute("oggvorbis -p -f " .. file) ~= DS.CMD_OK then
			os.execute("console --show");
		end
		
	elseif ext == ".adx" or ext == ".s3m" then
	
		local mod = string.sub(ext, -3);
	
		if not self.modal.visible then 
			return self:ShowModal("confirm", 'Play file "' .. name .. '" ?', "exec");
		else
			self:HideModal();
		end

		if not DS.GetCmdByName(mod) then
		
			 if not self:loadModule(mod) then
			 	os.execute("console --show");
			 	return file; 
			 end
		end
		
		if os.execute(mod .. " -p -f " .. file) ~= DS.CMD_OK then
			os.execute("console --show");
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
			 	os.execute("console --show");
			 	return file; 
			 end
		end
		
		if os.execute("ffplay -p -f " .. file) ~= DS.CMD_OK then
			os.execute("console --show");
		end
		
	elseif ext == ".opk" then
	
		if not self.modal.visible then 
			return self:ShowModal("confirm", 'Install "' .. name .. '" package?', "exec");
		else
			self:HideModal();
		end

		if not DS.GetCmdByName("opkg") then
		
			 if not self:loadModule("minilzo") or not self:loadModule("opkg") then
			 	os.execute("console --show");
			 	return file; 
			 end
		end
		
		os.execute("console --show\nopkg -i -f " .. file);
		os.execute("sleep 1000\nconsole --hide");
		
	else
	
		local app = DS.GetAppByExtension(ext);
		
		if app ~= nil then
		
			if not self.modal.visible then 
				return self:ShowModal("confirm", 'Open file in ' .. app.name .. ' app?', "exec");
			else
				self:HideModal();
			end
			
			self:openApp(app.name, file, 0); -- Unload filemanager?
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
	--local path = "";
	
	--if mgr ~= nil then
		local path = GUI.FileManagerGetPath(mgr.widget);
	--end
	
	return {file = path .. "/" .. mgr.ent.name, name = mgr.ent.name, path = path};
end


function FileManagerItemClickTop(ent)
	--print("FileManagerItemClickTop: " .. ent.fmwname);
	FileManager:ItemClick(ent, FileManager.mgr.top);
end

function FileManagerItemContextClickTop(ent)
	--print("FileManagerItemClickTop: " .. ent.fmwname);
	FileManager:ItemContextClick(ent, FileManager.mgr.top);
end

function FileManagerItemClickBottom(ent)
	--print("FileManagerItemClickBottom" .. ent.fmwname);
	FileManager:ItemClick(ent, FileManager.mgr.bottom);
end

function FileManagerItemContextClickBottom(ent)
	--print("FileManagerItemClickBottom" .. ent.fmwname);
	FileManager:ItemContextClick(ent, FileManager.mgr.bottom);
end


function FileManager:ItemClick(ent, mgr)
	
	self:focusManager(mgr);
	--print("Clicked: " .. ent.name .. " mgr: " .. mgr.id .. "\n");
	
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
	--print("Context clicked: " .. ent.name .. " mgr: " .. mgr.id .. "\n");
	--TODO
end


function FileManagerItemMouseover(ent)
	if ent.size > 0 then
		FileManager:tooltip(string.format("File size: %d KB", (ent.size / 1024)));
	end
end

--
--function FileManagerItemMouseout(ent)
--	if ent.size > 0 then
--		FileManager:tooltip(nil);
--	end
--end
--


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
            self:showError("FileManager:getResource: Uknown resource type - " .. type);
			return nil;
		end
	end

	self:showError("FileManager:getResource: Can't find resource - " .. name);
	return nil;
end


function FileManager:getElement(name)

	local r = DS.listGetItemByName(self.app.elements, name);

	if r ~= nil then
		return GUI.AnyToWidget(r.data);
	end

	self:showError("FileManager:getElement: Can't find element - " .. name);
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

			self.modal.widget = self:getElement("modal-win");
	
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
			
			self:HideModal();
			self:focusManager(self.mgr.top);
			self:toolbarModeSwitch();
			
			--print("File Manager initialized.\n");
		end
	end
end


function FileManager:Shutdown()
	self:unloadModules();
end


--end
