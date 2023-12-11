-----------------------------------------
--                                     --
-- @name:    Region Changer            --
-- @version: 1.8.5                     --
-- @author:  SWAT                      --
-- @url:     http://www.dc-swat.ru     --
--                                     --
-----------------------------------------


--if not RegionChanger then

	RegionChanger = {
		app = nil,
		pages = nil,
		progress = nil,
		swirl = nil,
		broadcast = {},
		country = {},
		lang = {},
		clear = {
			sys = nil,
			game = nil
		},
		data = nil,
		values = nil
	}

	
	function RegionChanger:ShowPage(index) 
		DS.ScreenFadeOutEx(nil, 1);
		GUI.CardStackShowIndex(self.pages, index); 
		DS.ScreenFadeIn();
	end
	
	
	function RegionChanger:isFileExists(file)
		local f = io.open(file);
		if f ~= nil then
			f:close();
			return true;
		end
		return false;
	end
	
	
	function RegionChanger:SelectBroadcast(index) 
	
		for i = 1, table.getn(self.broadcast) do

			if i == index then
				
				--if not GUI.WidgetGetState(self.broadcast[i]) then
					GUI.WidgetSetState(self.broadcast[i], 1); 
					RC.flash_factory_set_broadcast(self.data, index);
				--end
				
			else
				GUI.WidgetSetState(self.broadcast[i], 0); 
			end
		end
	end
	
	function RegionChanger:SelectCountry(index) 
		for i = 1, table.getn(self.country) do

			if i == index then
				
				--if not GUI.WidgetGetState(self.country[i]) then
					GUI.WidgetSetState(self.country[i], 1); 
					RC.flash_factory_set_country(self.data, index, GUI.WidgetGetState(self.swirl));
				--end
				
			else
				GUI.WidgetSetState(self.country[i], 0); 
			end
		end
	end

	
	function RegionChanger:SelectLang(index) 
	
		for i = 1, table.getn(self.lang) do

			if i == index then
				
				--if not GUI.WidgetGetState(self.lang[i]) then
					GUI.WidgetSetState(self.lang[i], 1);
					RC.flash_factory_set_lang(self.data, index);					
				--end
				
			else
				GUI.WidgetSetState(self.lang[i], 0); 
			end
		end
	end
	
	
	function RegionChanger:Read() 
	
		if self.data then
			RC.flash_factory_free_data(self.data);
			RC.flash_factory_free_values(self.values);
		end
	
		self.data = RC.flash_read_factory();
		self.values = RC.flash_factory_get_values(self.data);
		GUI.WidgetSetState(self.swirl, self.values.black_swirl);
		self:SelectCountry(self.values.country);
		self:SelectBroadcast(self.values.broadcast);
		self:SelectLang(self.values.lang);

	end
	
	
	function RegionChanger:Write() 
		RC.flash_write_factory(self.data);
	end
	
	
	function RegionChanger:ClearFlashrom()
		if GUI.WidgetGetState(self.clear.sys) then
			RC.flash_clear(KOS.FLASHROM_PT_SETTINGS);
		end
		if GUI.WidgetGetState(self.clear.game) then
			RC.flash_clear(KOS.FLASHROM_PT_BLOCK_1);
		end
	end
	
	
	function RegionChanger:ChangeProgress(p)
		GUI.ProgressBarSetPosition(self.progress, p);
	end
	
	function RegionChanger:CreateBackup()
	
		self:ChangeProgress(0.1);
		local file = os.getenv("PATH").."/firmware/flash/factory_backup.bin";
		
		if self:isFileExists(file) then
			os.remove(file);
		end
		
		local f = KOS.fs_open(file, KOS.O_WRONLY);
		self:ChangeProgress(0.2);
		local data = RC.flash_read_factory();
		self:ChangeProgress(0.4);
		
		if f > -1 then

			KOS.fs_write(f, data, 8192);
			self:ChangeProgress(0.6);
			KOS.fs_close(f);
			RC.flash_factory_free_data(data);
			self:ChangeProgress(0.8);
		
		else
		
			file = "/ram/flash_backup.bin";
			f = KOS.fs_open(file, KOS.O_WRONLY);
		
			if self:isFileExists("/vmu/a1/FLASHBKP.BIN") then
				os.remove("/vmu/a1/FLASHBKP.BIN");
			elseif self:isFileExists("/vmu/a2/FLASHBKP.BIN") then
				os.remove("/vmu/a2/FLASHBKP.BIN");
			end

			KOS.fs_write(f, data, 8192); 
			self:ChangeProgress(0.6);
			KOS.fs_close(f);
			RC.flash_factory_free_data(data);
			self:ChangeProgress(0.8);

			if os.execute("vmu -c -n -f "..file.." -o /vmu/a1/FLASHBKP.BIN -i Region Changer Backup") ~= DS.CMD_OK then
				os.execute("vmu -c -n -f "..file.." -o /vmu/a2/FLASHBKP.BIN -i Region Changer Backup");
			end
			os.remove(file);
		end
		self:ChangeProgress(1.0);
	end
	
	
	function RegionChanger:RestoreBackup()
	
		self:ChangeProgress(0.1);
		
		local file = os.getenv("PATH").."/firmware/flash/factory_backup.bin";
		local f = KOS.fs_open(file, KOS.O_RDONLY);
		local data = RC.flash_read_factory();
		self:ChangeProgress(0.2);
		
		if f > -1 then
		
			self:ChangeProgress(0.4);
			KOS.fs_read(f, data, 8192); 
			self:ChangeProgress(0.6);
			KOS.fs_close(f);
			RC.flash_write_factory(data);
			self:ChangeProgress(0.8);
			RC.flash_factory_free_data(data);
			self:Read();
		
		else
			
			file = "/ram/flash_backup.bin";
			if not os.execute("vmu -c -v -f "..file.." -o /vmu/a1/FLASHBKP.BIN") then
				os.execute("vmu -c -v -f "..file.." -o /vmu/a2/FLASHBKP.BIN");
			end
			
			self:ChangeProgress(0.4);
			f = KOS.fs_open(file, KOS.O_RDONLY);
			
			self:ChangeProgress(0.6);
			KOS.fs_read(f, data, 8192); 
			self:ChangeProgress(0.8);
			KOS.fs_close(f);
			RC.flash_write_factory(data);
			self:ChangeProgress(0.9);
			RC.flash_factory_free_data(data);
			os.remove(file);
			self:Read();
		end
		self:ChangeProgress(1.0);
	end
	
	
	function RegionChanger:GetElement(name)
		local el = DS.listGetItemByName(self.app.elements, name);

		if el ~= nil then
			return GUI.AnyToWidget(el.data);
		end
		return nil;
	end
	

	function RegionChanger:Initialize()

		if self.app == nil then
			
			self.app = DS.GetAppById(THIS_APP_ID);
			
			if self.app ~= nil then

				self.pages = self:GetElement("pages");
				self.progress = self:GetElement("backup-progress");
				self.swirl = self:GetElement("change-swirl-checkbox");
				self.clear.sys = self:GetElement("clear-sys-checkbox");
				self.clear.game = self:GetElement("clear-game-checkbox");
				
				table.insert(self.country, self:GetElement("change-country-japan-checkbox"));
				table.insert(self.country, self:GetElement("change-country-usa-checkbox"));
				table.insert(self.country, self:GetElement("change-country-europe-checkbox"));
				
				table.insert(self.broadcast, self:GetElement("change-ntsc-checkbox"));
				table.insert(self.broadcast, self:GetElement("change-pal-checkbox"));
				table.insert(self.broadcast, self:GetElement("change-palm-checkbox"));
				table.insert(self.broadcast, self:GetElement("change-paln-checkbox"));
				
				table.insert(self.lang, self:GetElement("change-lang-japan-checkbox"));
				table.insert(self.lang, self:GetElement("change-lang-english-checkbox"));
				table.insert(self.lang, self:GetElement("change-lang-german-checkbox"));
				table.insert(self.lang, self:GetElement("change-lang-french-checkbox"));
				table.insert(self.lang, self:GetElement("change-lang-spanish-checkbox"));
				table.insert(self.lang, self:GetElement("change-lang-italian-checkbox"));
				self:Read();
				
				return true;
			end
		end
	end 

--end