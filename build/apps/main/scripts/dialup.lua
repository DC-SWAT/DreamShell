-----------------------------------------
--                                     --
-- @name:     Dial-up connect script   --
-- @author:   SWAT                     --
-- @url:      http://www.dc-swat.ru    --
--                                     --
-----------------------------------------
--

ShowConsole();
Sleep(1000);
OpenModule(os.getenv("PATH") .. "/modules/ppp.klf");
os.execute("net -i");
print("Dialing...\n");
os.execute("ppp -i");
