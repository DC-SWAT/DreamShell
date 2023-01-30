-----------------------------------------
--                                     --
-- @name:     Dial-up connect script   --
-- @author:   SWAT                     --
-- @url:      http://www.dc-swat.ru    --
--                                     --
-----------------------------------------
--

ShowConsole();
Sleep(500);
print("To get back GUI press: Start, A, Start\n");
if OpenModule(os.getenv("PATH") .. "/modules/ppp.klf") then
    print("Dialing...\n");
    os.execute("ppp -i");
end
