-----------------------------------------
--                                     --
-- @name:     FTP server script        --
-- @author:   SWAT                     --
-- @url:      http://www.dc-swat.ru    --
--                                     --
-----------------------------------------
--

ShowConsole();
Sleep(500);
print("To get back GUI press: Start, A, Start\n");
if OpenModule(os.getenv("PATH") .. "/modules/ftpd.klf") then
    if os.execute("ftpd -s -p 21 -d /") == 0 then
        print("Network IPv4: " .. os.getenv("NET_IPV4") .. "\n");
    end
end
