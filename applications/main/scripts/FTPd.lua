-----------------------------------------
--                                     --
-- @name:     FTP server script        --
-- @author:   SWAT                     --
-- @url:      http://www.dc-swat.ru    --
--                                     --
-----------------------------------------
--

ShowConsole();
print("To get back GUI press: Start, A, Start\n");
if OpenModule(os.getenv("PATH") .. "/modules/ftpd.klf") then
    os.execute("ftpd -s -p 21 -d /");
end
