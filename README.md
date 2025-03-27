SCClone - Windows Service Controller Utility
SCClone is a command-line utility for managing Windows services. It provides similar functionality to the Windows built-in sc.exe tool and was written for a school assignment. A file named 'test_service.exe' is provided for testing purposes. Note that test_service.exe has a check for running as a service, hence the '--service' in the create example.

Compiling
Command Line/Linux: Type 'g++ [program_name.cpp]'

Windows using Visual Studio: You can right click on the project and click 'build' or click the green debugger button at the top (or the green arrow next to it). This will create an .exe file in the debug folder.


Supported Commands

query - Queries service status
create - Creates a service
qdescription - Queries service description
start - Starts a service
stop - Stops a service
delete - Deletes a service
config - Modifies service configuration
failure - Sets service failure actions


General syntax

scclone.exe [command] [/parameter] [parameter value] [/parameter] [parameter value]...

Syntax for Windows native sc.exe can be found here: https://learn.microsoft.com/en-us/previous-versions/windows/it-pro/windows-server-2012-r2-and-2012/cc754599(v=ws.11). Scclone.exe has the same parameters and options.

scclone.exe uses the same syntax, except parameters have a / in front. Additionally, when using the 'interact' type, the syntax is /type "interact type=own" or "interact type=share". This is because /type interact alone is not sufficient, as 'interact' has additional parameters.

Use quotes "" around filepaths and anything that has a space in it to have it properly processed as a parameter.

Example command series:
scclone.exe (Just executing the file will bring up a supported commands list)
scclone.exe create /servicename TestService /binpath "C:\Users\User\Desktop\test_service.exe --service" /displayname "TestService"
scclone.exe create /servicename TestFullService /binpath "C:\Users\User\Desktop\test_service.exe --service" /displayname "TestService" /type own /start auto /error critical /group NetworkProvider /tag yes /depend RPCSS/Tcpip /obj LocalSystem /password "" /description "This is a test service that uses all parameters" (note that 'config' syntax is the same except without the /tag option)
scclone.exe query TestService
scclone.exe failure TestService /reset 30 /actions "restart/2000" /command "C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe"
sc qc TestService and 'sc qfailure TestService' (this file does not contain functionality for 'qc' which shows config info or qfailure which shows failure info)
scclone.exe start TestService
scclone.exe qdescription TestService
scclone.exe stop TestService
scclone.exe delete TestService



Parameters

For Query Command

None, just use scclone.exe query [target service]

For Config Command

/servicename - Name of the service
/displayname - Display name for the service
/binpath - Path to the service executable
/start - When the service starts (auto, demand, disabled, boot, system, delayed-auto)
/type - Service type (own, share, kernel, filesys, interact)
/error - Error control (normal, severe, critical, ignore)
/obj - Account under which the service runs
/password - Password for the service account
/description - Description of the service
/depend - Services this service depends on

For Create Command

Create has the same parameters as Config with the addition of /tag. This parameter cannot be changed after creation and so is not available for Config.

For failure Command

/reset - Time in seconds to reset failure count (default: 86400 - 1 day)
/reboot - Message to broadcast on reboot
/command - Command to run on failure
/actions - List of actions to take (format: action1/delay1/action2/delay2/...)

Actions: run, restart, reboot, none
Delays are in seconds

For Start, Stop, Delete, and qdescription Commands

None, just use scclone.exe start/stop/delete/qdescription [target service] 

Notes

Many operations require elevated privileges. Run SCClone as an administrator for full functionality. Generated using a lot of Claude AI, copy code at your own risk.
