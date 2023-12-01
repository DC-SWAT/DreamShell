-----------------------------------------
--                                     --
-- @name:     Dial-up connect script   --
-- @author:   SWAT                     --
-- @url:      http://www.dc-swat.ru    --
--                                     --
-----------------------------------------
--

ShowConsole();
print("To get back GUI press: Start, A, Start\n");
print("Initializing LAN network...\n");
os.execute("net --init");
Sleep(2000);
HideConsole();
