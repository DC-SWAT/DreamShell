@Echo off
SET PATH=sys
echo Creating ISO...
mkisofs -V ISOLDR -G data/IP.BIN -joliet -rock -l -o isoldr_game.iso ./data
echo Complete!