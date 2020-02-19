
Config.cfg contains basic configuration data and needs to be in the same directory as scpFwd.exe.
Update admin email as appropriate.
Process needs to run on a server that supports sendmail.
Make sure the desired port is open in the firewall.

Files will import to "Output/[called aetitle]"
Copy or create the appropriate config as "Output/[called aetitle]/myname.cfg"
Keyed data will be stored in "Output/[called aetitle]/test.map"
test.map can be prepopulated with known/desired keys

A cron job must be created to flush the Finished folder or it will retain files indefinitely.

Requires the DCMTK libraries be built/provided.
