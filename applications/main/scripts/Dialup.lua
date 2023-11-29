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
    if os.execute("ppp -i") == 0 then
        print("Network IPv4: " .. os.getenv("NET_IPV4") .. "\n");
    end
end
