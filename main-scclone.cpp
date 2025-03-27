/*To do:
- query works but it has several outputs that don't match the output from sc.exe, current ones useful but could still change
- config
- failure
- create
scclone.exe create /servicename TestService /binpath "C:\Users\User\Desktop\test_service.exe --service" /displayname "Test Service"
worked for create, make sure other params are included


Completed:
- Start validated functioning correctly
- Stop validated functioning correctly
- qdescription validated functioning correctly
- Delete validated functioning correctly
*/

/*Usage:

For paramters, type /parameter [value]. For example:
scclone.exe create /servicename TestService /binpath "C:\Users\User\Desktop\test_service.exe --service" /displayname "Test Service"

*/


#include <windows.h>    // Windows API functions and data types
#include <iostream>     // For input/output stream operations
#include <string>       // For string handling
#include <vector>       // For dynamic arrays
#include <map>          // For key-value storage




//=============================================================================
// Helper functions - These provide utility functionality used throughout the program
//=============================================================================

/**
 * Displays usage information for the program
 */
void PrintUsage() {
    std::cout << "SC Clone - Service Controller utility\n";
    std::cout << "Usage: scclone <command> [options]\n\n";
    std::cout << "Supported commands:\n";
    std::cout << "  query         - Queries service status\n";
    std::cout << "  create        - Creates a service\n";
    std::cout << "  qdescription  - Queries service description\n";
    std::cout << "  start         - Starts a service\n";
    std::cout << "  stop          - Stops a service\n";
    std::cout << "  delete        - Deletes a service\n";
    std::cout << "  config        - Modifies service configuration\n";
    std::cout << "  failure       - Sets service failure actions\n";
}

/**
 * Converts a standard string to a wide string
 * This is necessary for many Windows API functions that require wide strings (UTF-16)
 *
 * @param str The string to convert
 * @return The equivalent wide string
 */
std::wstring StringToWString(const std::string& str) {
    if (str.empty()) return std::wstring();
    // Calculate the required buffer size
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstr(size_needed, 0);
    // Perform the actual conversion
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstr[0], size_needed);
    return wstr;
}

/**
 * Converts a wide string to a standard string
 * Used to convert Windows API wide string results back to standard strings
 *
 * @param wstr The wide string to convert
 * @return The equivalent standard string
 */
std::string WStringToString(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    // Calculate the required buffer size
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string str(size_needed, 0);
    // Perform the actual conversion
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &str[0], size_needed, NULL, NULL);
    return str;
}

/**
 * Gets a human-readable error message for the last Windows API error
 * Used to provide meaningful error messages when API calls fail
 *
 * @return String containing the error message
 */
std::string GetLastErrorAsString() {
    DWORD error = GetLastError();
    if (error == 0) return "No error";

    LPSTR messageBuffer = nullptr;
    // Format the error message
    size_t size = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

    std::string message(messageBuffer, size);
    // Free the buffer allocated by FormatMessage
    LocalFree(messageBuffer);
    return message;
}

/**
 * Parses command line arguments into a key-value map
 * Arguments are expected in the form /key=value or /key value
 *
 * @param argc Number of command line arguments
 * @param argv Array of command line arguments
 * @param startIdx Index to start parsing from
 * @return Map of parameter names to values
 */
std::map<std::string, std::string> ParseArgs(int argc, char* argv[], int startIdx) {
    std::map<std::string, std::string> args;

    for (int i = startIdx; i < argc; i++) {
        std::string arg = argv[i];
        if (arg[0] == '/') {
            // Extract parameter name
            std::string name = arg.substr(1);
            std::string value;

            // Check if the next argument is a value (not starting with /)
            if (i + 1 < argc && argv[i + 1][0] != '/') {
                value = argv[i + 1];
                i++; // Skip the value in the next iteration
            }

            args[name] = value;
        }
    }

    return args;
}

/**
 * Converts a service state code to a human-readable string
 *
 * @param state Service state code from SERVICE_STATUS
 * @return String representation of the service state
 */
std::string GetServiceStateString(DWORD state) {
    switch (state) {
    case SERVICE_STOPPED: return "STOPPED";
    case SERVICE_START_PENDING: return "START_PENDING";
    case SERVICE_STOP_PENDING: return "STOP_PENDING";
    case SERVICE_RUNNING: return "RUNNING";
    case SERVICE_CONTINUE_PENDING: return "CONTINUE_PENDING";
    case SERVICE_PAUSE_PENDING: return "PAUSE_PENDING";
    case SERVICE_PAUSED: return "PAUSED";
    default: return "UNKNOWN";
    }
}

/**
 * Converts a service type code to a human-readable string
 * Service types can be combinations of flags, so this function handles that
 *
 * @param type Service type code from SERVICE_STATUS
 * @return String representation of the service type
 */
std::string GetServiceTypeString(DWORD type) {
    std::string result;

    // Check each possible flag and add to result if present
    if (type & SERVICE_KERNEL_DRIVER) result += "KERNEL_DRIVER ";
    if (type & SERVICE_FILE_SYSTEM_DRIVER) result += "FILE_SYSTEM_DRIVER ";
    if (type & SERVICE_WIN32_OWN_PROCESS) result += "WIN32_OWN_PROCESS ";
    if (type & SERVICE_WIN32_SHARE_PROCESS) result += "WIN32_SHARE_PROCESS ";
    if (type & SERVICE_INTERACTIVE_PROCESS) result += "INTERACTIVE_PROCESS ";

    if (result.empty()) result = "UNKNOWN";
    else result.pop_back(); // Remove trailing space

    return result;
}

/**
 * Converts a service start type code to a human-readable string
 *
 * @param startType Service start type code
 * @return String representation of the service start type
 */
std::string GetServiceStartTypeString(DWORD startType) {
    switch (startType) {
    case SERVICE_BOOT_START: return "BOOT";
    case SERVICE_SYSTEM_START: return "SYSTEM";
    case SERVICE_AUTO_START: return "AUTO";
    case SERVICE_DEMAND_START: return "DEMAND";
    case SERVICE_DISABLED: return "DISABLED";
    default: return "UNKNOWN";
    }
}

//=============================================================================
// Command implementations - These implement the actual service control commands
//=============================================================================

/**
 * Queries and displays detailed information about a Windows service
 * Similar to "sc query <service>"
 *
 * @param serviceName Name of the service to query
 * @return true if successful, false otherwise
 */
bool QueryService(const std::string& serviceName) {
    // Open a handle to the service control manager
    SC_HANDLE scManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (!scManager) {
        std::cerr << "Failed to open service control manager: " << GetLastErrorAsString() << std::endl;
        return false;
    }

    // Open a handle to the specified service
    SC_HANDLE service = OpenServiceA(scManager, serviceName.c_str(), SERVICE_QUERY_STATUS | SERVICE_QUERY_CONFIG);
    if (!service) {
        std::cerr << "Failed to open service: " << GetLastErrorAsString() << std::endl;
        CloseServiceHandle(scManager);
        return false;
    }

    // Get service status information
    SERVICE_STATUS_PROCESS status;
    DWORD bytesNeeded;
    if (!QueryServiceStatusEx(service, SC_STATUS_PROCESS_INFO, (LPBYTE)&status, sizeof(status), &bytesNeeded)) {
        std::cerr << "Failed to query service status: " << GetLastErrorAsString() << std::endl;
        CloseServiceHandle(service);
        CloseServiceHandle(scManager);
        return false;
    }

    // Get service config - first call to get required buffer size
    DWORD bytesNeeded2 = 0;
    BOOL result = QueryServiceConfig(service, NULL, 0, &bytesNeeded2);
    // This call is expected to fail with ERROR_INSUFFICIENT_BUFFER
    if (!result && GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
        std::cerr << "Failed to determine buffer size for service config: " << GetLastErrorAsString() << std::endl;
        CloseServiceHandle(service);
        CloseServiceHandle(scManager);
        return false;
    }
    std::vector<BYTE> buffer(bytesNeeded2);
    LPQUERY_SERVICE_CONFIG config = (LPQUERY_SERVICE_CONFIG)buffer.data();

    // Call QueryServiceConfig again to fill the buffer with data
    // Added to fix issue where 'TYPE' kept showing up as UNKNOWN, issue was that I didn't call this again after previous expected fail
    if (!QueryServiceConfig(service, config, bytesNeeded2, &bytesNeeded2)) {
        std::cerr << "Failed to query service config: " << GetLastErrorAsString() << std::endl;
        CloseServiceHandle(service);
        CloseServiceHandle(scManager);
        return false;
    }

    // Convert the wide string pointers to regular strings
    //config->lpBinaryPathName is a wide string pointer (LPWSTR) which can't be printed to a stream expecting narrow characters
    //Windows API is using unicode even though I'm calling ANSI functions
    //So to fix, have to convert wide strings to regular strings
    std::string displayName = "(none)";
    if (config->lpDisplayName) {
        displayName = WStringToString(std::wstring(config->lpDisplayName));
    }

    std::string binaryPath = "(none)";
    if (config->lpBinaryPathName) {
        binaryPath = WStringToString(std::wstring(config->lpBinaryPathName));
    }


    // Display service information
    std::cout << "DISPLAY_NAME: " << displayName << std::endl;
    std::cout << "TYPE        : " << GetServiceTypeString(config->dwServiceType) << std::endl;
    std::cout << "START_TYPE  : " << GetServiceStartTypeString(config->dwStartType) << std::endl;
    std::cout << "STATE       : " << GetServiceStateString(status.dwCurrentState) << std::endl;
    std::cout << "PID         : " << status.dwProcessId << std::endl;
    std::cout << "BINARY_PATH : " << binaryPath << std::endl;



    // Clean up resources
    CloseServiceHandle(service);
    CloseServiceHandle(scManager);
    return true;
}

/**
 * Creates a new Windows service
 * Similar to "sc create <service> ..."
 *
 * @param args Map of parameters for the new service
 * @return true if successful, false otherwise
 */
bool CreateService(const std::map<std::string, std::string>& args) {
    // Check for required parameters
    if (args.find("servicename") == args.end() || args.find("binpath") == args.end()) {
        std::cerr << "ERROR: Missing required parameters. Required: /servicename and /binpath" << std::endl;
        return false;
    }

    std::string serviceName = args.at("servicename");
    std::string binPath = args.at("binpath");
    std::string displayName = args.count("displayname") ? args.at("displayname") : serviceName;

    // Determine start type (default is demand start)
    DWORD startType = SERVICE_DEMAND_START; // Default
    if (args.count("start")) {
        std::string start = args.at("start");
        if (start == "boot") startType = SERVICE_BOOT_START;
        else if (start == "system") startType = SERVICE_SYSTEM_START;
        else if (start == "auto") startType = SERVICE_AUTO_START;
        else if (start == "demand") startType = SERVICE_DEMAND_START;
        else if (start == "disabled") startType = SERVICE_DISABLED;
        else if (start == "delayed-auto") startType = SERVICE_AUTO_START; // We'll handle delayed-auto separately
    }

    // Determine service type (default is own process)
    DWORD serviceType = SERVICE_WIN32_OWN_PROCESS; // Default
    if (args.count("type")) {
        std::string type = args.at("type");
        if (type == "own") serviceType = SERVICE_WIN32_OWN_PROCESS;
        else if (type == "share") serviceType = SERVICE_WIN32_SHARE_PROCESS;
        else if (type == "kernel") serviceType = SERVICE_KERNEL_DRIVER;
        else if (type == "filesys") serviceType = SERVICE_FILE_SYSTEM_DRIVER;
        else if (type == "rec") serviceType = SERVICE_FILE_SYSTEM_DRIVER | SERVICE_RECOGNIZER_DRIVER;
        else if (type == "interact type=own") serviceType = SERVICE_INTERACTIVE_PROCESS | SERVICE_WIN32_OWN_PROCESS;
        else if (type == "interact type=share") serviceType = SERVICE_INTERACTIVE_PROCESS | SERVICE_WIN32_SHARE_PROCESS;
        else if (type == "interact") {
            std::cerr << "ERROR: 'interact' type must be used with 'own' or 'share' (e.g., type=interact type=own)" << std::endl;
            return false;
        }
    }

    // Determine error control (default is normal)
    DWORD errorControl = SERVICE_ERROR_NORMAL; // Default
    if (args.count("error")) {
        std::string error = args.at("error");
        if (error == "normal") errorControl = SERVICE_ERROR_NORMAL;
        else if (error == "severe") errorControl = SERVICE_ERROR_SEVERE;
        else if (error == "critical") errorControl = SERVICE_ERROR_CRITICAL;
        else if (error == "ignore") errorControl = SERVICE_ERROR_IGNORE;
    }

    // Get load ordering group if provided
    LPSTR loadOrderGroup = NULL;
    std::string groupStr;
    if (args.count("group")) {
        groupStr = args.at("group");
        loadOrderGroup = const_cast<LPSTR>(groupStr.c_str());
    }

    // Determine if a tag should be obtained
    LPDWORD tagId = NULL;
    DWORD tag = 0;
    if (args.count("tag") && args.at("tag") == "yes") {
        tagId = &tag;
    }

    // Process dependencies
    LPSTR dependencies = NULL;
    std::string dependStr;
    if (args.count("depend")) {
        // Convert forward slashes to null characters and add double null at end
        dependStr = args.at("depend");
        size_t pos = 0;
        while ((pos = dependStr.find('/', pos)) != std::string::npos) {
            dependStr.replace(pos, 1, 1, '\0');
            pos++;
        }
        dependStr.push_back('\0'); // Add terminating null
        dependencies = const_cast<LPSTR>(dependStr.c_str());
    }

    // Get account name and password
    LPSTR accountName = NULL;
    LPSTR password = NULL;
    std::string accountStr;
    std::string passwordStr;

    if (args.count("obj")) {
        accountStr = args.at("obj");
        accountName = const_cast<LPSTR>(accountStr.c_str());
    }

    if (args.count("password")) {
        passwordStr = args.at("password");
        password = const_cast<LPSTR>(passwordStr.c_str());
    }

    // Open the service control manager
    SC_HANDLE scManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (!scManager) {
        std::cerr << "Failed to open service control manager: " << GetLastErrorAsString() << std::endl;
        return false;
    }

    // Create the service
    SC_HANDLE service = CreateServiceA(
        scManager,                       // SCM handle
        serviceName.c_str(),             // Service name
        displayName.c_str(),             // Display name
        SERVICE_ALL_ACCESS,              // Desired access
        serviceType,                     // Service type
        startType,                       // Start type
        errorControl,                    // Error control
        binPath.c_str(),                 // Binary path
        loadOrderGroup,                  // Load ordering group
        tagId,                           // Tag ID
        dependencies,                    // Dependencies
        accountName,                     // Account name
        password                         // Password
    );

    if (!service) {
        std::cerr << "Failed to create service: " << GetLastErrorAsString() << std::endl;
        CloseServiceHandle(scManager);
        return false;
    }

    std::cout << "Service created successfully: " << serviceName << std::endl;

    // Set description if provided
    if (args.count("description")) {
        SERVICE_DESCRIPTIONA desc = { 0 };
        desc.lpDescription = const_cast<LPSTR>(args.at("description").c_str());

        // Use ChangeServiceConfig2 to set the description
        if (!ChangeServiceConfig2A(service, SERVICE_CONFIG_DESCRIPTION, &desc)) {
            std::cerr << "Warning: Failed to set service description: " << GetLastErrorAsString() << std::endl;
        }
    }

    // Set delayed auto-start if specified
    if (args.count("start") && args.at("start") == "delayed-auto") {
        SERVICE_DELAYED_AUTO_START_INFO delayedInfo = { TRUE };
        if (!ChangeServiceConfig2A(service, SERVICE_CONFIG_DELAYED_AUTO_START_INFO, &delayedInfo)) {
            std::cerr << "Warning: Failed to set delayed auto-start: " << GetLastErrorAsString() << std::endl;
        }
    }

    // If tag was requested, display it
    if (tagId) {
        std::cout << "Tag ID: " << tag << std::endl;
    }

    // Clean up resources
    CloseServiceHandle(service);
    CloseServiceHandle(scManager);
    return true;
}

/**
 * Queries and displays the description of a Windows service
 * Similar to "sc qdescription <service>"
 *
 * @param serviceName Name of the service to query
 * @return true if successful, false otherwise
 */
bool QueryServiceDescription(const std::string& serviceName) {
    // Open a handle to the service control manager
    SC_HANDLE scManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (!scManager) {
        std::cerr << "Failed to open service control manager: " << GetLastErrorAsString() << std::endl;
        return false;
    }

    // Open a handle to the specified service
    SC_HANDLE service = OpenServiceA(scManager, serviceName.c_str(), SERVICE_QUERY_CONFIG);
    if (!service) {
        std::cerr << "Failed to open service: " << GetLastErrorAsString() << std::endl;
        CloseServiceHandle(scManager);
        return false;
    }

    // Query service description - first call to get required buffer size
    DWORD bytesNeeded = 0;
    SERVICE_DESCRIPTIONA minimalBuffer = { 0 };
    BOOL result = QueryServiceConfig2A(service, SERVICE_CONFIG_DESCRIPTION,
        (LPBYTE)&minimalBuffer, sizeof(SERVICE_DESCRIPTIONA), &bytesNeeded);
    // This call is still expected to fail if more space is needed
    if (!result && GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
        std::cerr << "Failed to query service description: " << GetLastErrorAsString() << std::endl;
        CloseServiceHandle(service);
        CloseServiceHandle(scManager);
        return false;
    }

    // Allocate buffer and get the description data
    std::vector<BYTE> buffer(bytesNeeded);
    LPSERVICE_DESCRIPTIONA desc = (LPSERVICE_DESCRIPTIONA)buffer.data();

    if (!QueryServiceConfig2A(service, SERVICE_CONFIG_DESCRIPTION, buffer.data(), bytesNeeded, &bytesNeeded)) {
        std::cerr << "Failed to query service description: " << GetLastErrorAsString() << std::endl;
        CloseServiceHandle(service);
        CloseServiceHandle(scManager);
        return false;
    }

    // Display the service description
    std::cout << "SERVICE_NAME: " << serviceName << std::endl;
    std::cout << "DESCRIPTION: " << (desc->lpDescription ? desc->lpDescription : "(no description)") << std::endl;

    // Clean up resources
    CloseServiceHandle(service);
    CloseServiceHandle(scManager);
    return true;
}

/**
 * Starts a Windows service and waits for it to reach running state
 * Similar to "sc start <service>"
 *
 * @param serviceName Name of the service to start
 * @return true if successful, false otherwise
 */
bool StartService(const std::string& serviceName) {
    // Open a handle to the service control manager
    SC_HANDLE scManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (!scManager) {
        std::cerr << "Failed to open service control manager: " << GetLastErrorAsString() << std::endl;
        return false;
    }

    // Open a handle to the specified service
    SC_HANDLE service = OpenServiceA(scManager, serviceName.c_str(), SERVICE_START);
    if (!service) {
        std::cerr << "Failed to open service: " << GetLastErrorAsString() << std::endl;
        CloseServiceHandle(scManager);
        return false;
    }

    // Attempt to start the service
    if (!StartServiceA(service, 0, NULL)) {
        std::cerr << "Failed to start service: " << GetLastErrorAsString() << std::endl;
        CloseServiceHandle(service);
        CloseServiceHandle(scManager);
        return false;
    }

    std::cout << "Service start pending... " << std::endl;

    // Wait for service to start
    SERVICE_STATUS_PROCESS status;
    DWORD bytesNeeded;
    ULONGLONG startTime = GetTickCount64();
    DWORD waitTime = 30000; // 30 seconds timeout

    // Poll the service status until it's running or times out
    while (true) {
        if (!QueryServiceStatusEx(service, SC_STATUS_PROCESS_INFO, (LPBYTE)&status, sizeof(status), &bytesNeeded)) {
            std::cerr << "Failed to query service status: " << GetLastErrorAsString() << std::endl;
            CloseServiceHandle(service);
            CloseServiceHandle(scManager);
            return false;
        }

        // Check if service has reached the running state
        if (status.dwCurrentState == SERVICE_RUNNING) {
            std::cout << "Service started successfully." << std::endl;
            break;
        }

        // Check for timeout
        if (GetTickCount64() - startTime > waitTime) {
            std::cerr << "Service start timed out." << std::endl;
            CloseServiceHandle(service);
            CloseServiceHandle(scManager);
            return false;
        }

        // Wait a short time before checking again
        Sleep(250);
    }

    // Clean up resources
    CloseServiceHandle(service);
    CloseServiceHandle(scManager);
    return true;
}

/**
 * Stops a Windows service and waits for it to reach stopped state
 * Similar to "sc stop <service>"
 *
 * @param serviceName Name of the service to stop
 * @return true if successful, false otherwise
 */
bool StopService(const std::string& serviceName) {
    // Open a handle to the service control manager
    SC_HANDLE scManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (!scManager) {
        std::cerr << "Failed to open service control manager: " << GetLastErrorAsString() << std::endl;
        return false;
    }

    // Open a handle to the specified service
    SC_HANDLE service = OpenServiceA(scManager, serviceName.c_str(), SERVICE_STOP | SERVICE_QUERY_STATUS);
    if (!service) {
        std::cerr << "Failed to open service: " << GetLastErrorAsString() << std::endl;
        CloseServiceHandle(scManager);
        return false;
    }

    // Get current service status
    SERVICE_STATUS_PROCESS status;
    DWORD bytesNeeded;

    if (!QueryServiceStatusEx(service, SC_STATUS_PROCESS_INFO, (LPBYTE)&status, sizeof(status), &bytesNeeded)) {
        std::cerr << "Failed to query service status: " << GetLastErrorAsString() << std::endl;
        CloseServiceHandle(service);
        CloseServiceHandle(scManager);
        return false;
    }

    // Check if service is already stopped
    if (status.dwCurrentState == SERVICE_STOPPED) {
        std::cout << "Service is already stopped." << std::endl;
        CloseServiceHandle(service);
        CloseServiceHandle(scManager);
        return true;
    }

    // Send stop control code to the service
    SERVICE_STATUS svcStatus;
    if (!ControlService(service, SERVICE_CONTROL_STOP, &svcStatus)) {
        std::cerr << "Failed to stop service: " << GetLastErrorAsString() << std::endl;
        CloseServiceHandle(service);
        CloseServiceHandle(scManager);
        return false;
    }

    std::cout << "Service stop pending... " << std::endl;

    // Wait for service to stop
    ULONGLONG startTime = GetTickCount64();
    DWORD waitTime = 30000; // 30 seconds timeout

    // Poll the service status until it's stopped or times out
    while (true) {
        if (!QueryServiceStatusEx(service, SC_STATUS_PROCESS_INFO, (LPBYTE)&status, sizeof(status), &bytesNeeded)) {
            std::cerr << "Failed to query service status: " << GetLastErrorAsString() << std::endl;
            CloseServiceHandle(service);
            CloseServiceHandle(scManager);
            return false;
        }

        // Check if service has reached the stopped state
        if (status.dwCurrentState == SERVICE_STOPPED) {
            std::cout << "Service stopped successfully." << std::endl;
            break;
        }

        // Check for timeout
        if (GetTickCount64() - startTime > waitTime) {
            std::cerr << "Service stop timed out." << std::endl;
            CloseServiceHandle(service);
            CloseServiceHandle(scManager);
            return false;
        }

        // Wait a short time before checking again
        Sleep(250);
    }

    // Clean up resources
    CloseServiceHandle(service);
    CloseServiceHandle(scManager);
    return true;
}

/**
 * Deletes a Windows service
 * Similar to "sc delete <service>"
 *
 * @param serviceName Name of the service to delete
 * @return true if successful, false otherwise
 */
bool DeleteService(const std::string& serviceName) {
    // Open a handle to the service control manager
    SC_HANDLE scManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (!scManager) {
        std::cerr << "Failed to open service control manager: " << GetLastErrorAsString() << std::endl;
        return false;
    }

    // Open a handle to the specified service
    SC_HANDLE service = OpenServiceA(scManager, serviceName.c_str(), DELETE);
    if (!service) {
        std::cerr << "Failed to open service: " << GetLastErrorAsString() << std::endl;
        CloseServiceHandle(scManager);
        return false;
    }

    // Delete the service
    // Note: We use ::DeleteService to avoid confusion with our function name
    if (!::DeleteService(service)) {
        std::cerr << "Failed to delete service: " << GetLastErrorAsString() << std::endl;
        CloseServiceHandle(service);
        CloseServiceHandle(scManager);
        return false;
    }

    std::cout << "Service deleted successfully: " << serviceName << std::endl;

    // Clean up resources
    CloseServiceHandle(service);
    CloseServiceHandle(scManager);
    return true;
}

/**
 * Modifies the configuration of a Windows service
 * Similar to "sc config <service> ..."
 *
 * @param serviceName Name of the service to configure
 * @param args Map of configuration parameters to modify
 * @return true if successful, false otherwise
 */
bool ConfigService(const std::string& serviceName, const std::map<std::string, std::string>& args) {
    // Open a handle to the service control manager
    SC_HANDLE scManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (!scManager) {
        std::cerr << "Failed to open service control manager: " << GetLastErrorAsString() << std::endl;
        return false;
    }

    // Open a handle to the specified service
    SC_HANDLE service = OpenServiceA(scManager, serviceName.c_str(), SERVICE_CHANGE_CONFIG);
    if (!service) {
        std::cerr << "Failed to open service: " << GetLastErrorAsString() << std::endl;
        CloseServiceHandle(scManager);
        return false;
    }

    // Prepare config parameters (default is no change)
    LPSTR displayName = NULL;
    DWORD serviceType = SERVICE_NO_CHANGE;
    DWORD startType = SERVICE_NO_CHANGE;
    DWORD errorControl = SERVICE_NO_CHANGE;
    LPSTR binaryPathName = NULL;
    LPSTR loadOrderGroup = NULL;
    LPSTR dependencies = NULL;
    LPSTR serviceAccount = NULL;
    LPSTR password = NULL;
    std::string groupStr;
    std::string dependStr;
    std::string accountStr;
    std::string passwordStr;

    // Set displayName if provided
    if (args.count("displayname")) {
        displayName = const_cast<LPSTR>(args.at("displayname").c_str());
    }

    // Set service type if provided
    if (args.count("type")) {
        std::string type = args.at("type");
        if (type == "own") serviceType = SERVICE_WIN32_OWN_PROCESS;
        else if (type == "share") serviceType = SERVICE_WIN32_SHARE_PROCESS;
        else if (type == "kernel") serviceType = SERVICE_KERNEL_DRIVER;
        else if (type == "filesys") serviceType = SERVICE_FILE_SYSTEM_DRIVER;
        else if (type == "rec") serviceType = SERVICE_FILE_SYSTEM_DRIVER | SERVICE_RECOGNIZER_DRIVER;
        else if (type == "interact type=own") serviceType = SERVICE_INTERACTIVE_PROCESS | SERVICE_WIN32_OWN_PROCESS;
        else if (type == "interact type=share") serviceType = SERVICE_INTERACTIVE_PROCESS | SERVICE_WIN32_SHARE_PROCESS;


    }

    // Set startType if provided
    if (args.count("start")) {
        std::string start = args.at("start");
        if (start == "boot") startType = SERVICE_BOOT_START;
        else if (start == "system") startType = SERVICE_SYSTEM_START;
        else if (start == "auto") startType = SERVICE_AUTO_START;
        else if (start == "demand") startType = SERVICE_DEMAND_START;
        else if (start == "disabled") startType = SERVICE_DISABLED;
        // We'll handle delayed-auto separately after ChangeServiceConfig
    }

    // Set error control if provided
    if (args.count("error")) {
        std::string error = args.at("error");
        if (error == "normal") errorControl = SERVICE_ERROR_NORMAL;
        else if (error == "severe") errorControl = SERVICE_ERROR_SEVERE;
        else if (error == "critical") errorControl = SERVICE_ERROR_CRITICAL;
        else if (error == "ignore") errorControl = SERVICE_ERROR_IGNORE;
    }

    // Set binaryPathName if provided
    if (args.count("binpath")) {
        binaryPathName = const_cast<LPSTR>(args.at("binpath").c_str());
    }

    // Set load ordering group if provided
    if (args.count("group")) {
        groupStr = args.at("group");
        loadOrderGroup = const_cast<LPSTR>(groupStr.c_str());
    }

    // Process dependencies if provided
    if (args.count("depend")) {
        // Convert forward slashes to null characters and add double null at end
        dependStr = args.at("depend");
        size_t pos = 0;
        while ((pos = dependStr.find('/', pos)) != std::string::npos) {
            dependStr.replace(pos, 1, 1, '\0');
            pos++;
        }
        dependStr.push_back('\0'); // Add terminating null
        dependencies = const_cast<LPSTR>(dependStr.c_str());
    }

    // Set account name if provided
    if (args.count("obj")) {
        accountStr = args.at("obj");
        serviceAccount = const_cast<LPSTR>(accountStr.c_str());
    }

    // Set password if provided
    if (args.count("password")) {
        passwordStr = args.at("password");
        password = const_cast<LPSTR>(passwordStr.c_str());
    }


    // Debug output for service type
    std::cout << "Debug - Service Type (numeric value): " << serviceType << std::endl;
    std::cout << "Debug - Service Type (flags): ";
    if (serviceType == SERVICE_NO_CHANGE) std::cout << "SERVICE_NO_CHANGE";
    else {
        if (serviceType & SERVICE_KERNEL_DRIVER) std::cout << "SERVICE_KERNEL_DRIVER ";
        if (serviceType & SERVICE_FILE_SYSTEM_DRIVER) std::cout << "SERVICE_FILE_SYSTEM_DRIVER ";
        if (serviceType & SERVICE_WIN32_OWN_PROCESS) std::cout << "SERVICE_WIN32_OWN_PROCESS ";
        if (serviceType & SERVICE_WIN32_SHARE_PROCESS) std::cout << "SERVICE_WIN32_SHARE_PROCESS ";
        if (serviceType & SERVICE_INTERACTIVE_PROCESS) std::cout << "SERVICE_INTERACTIVE_PROCESS ";
    }
    std::cout << std::endl;

    // Also print the raw type string for debugging
    std::cout << "Debug - Type string from args: '" << args.at("type") << "'" << std::endl;


    // Call ChangeServiceConfig to apply changes
    if (!ChangeServiceConfigA(
        service,            // Service handle
        serviceType,        // Service type
        startType,          // Start type
        errorControl,       // Error control
        binaryPathName,     // Binary path
        loadOrderGroup,     // Load ordering group
        NULL,               // Tag ID (not changing - can't be changed after creation)
        dependencies,       // Dependencies
        serviceAccount,     // Account name
        password,           // Password
        displayName         // Display name
    )) {
        std::cerr << "Failed to configure service: " << GetLastErrorAsString() << std::endl;
        CloseServiceHandle(service);
        CloseServiceHandle(scManager);
        return false;
    }

    // Set description if provided
    if (args.count("description")) {
        SERVICE_DESCRIPTIONA desc = { 0 };
        desc.lpDescription = const_cast<LPSTR>(args.at("description").c_str());
        if (!ChangeServiceConfig2A(service, SERVICE_CONFIG_DESCRIPTION, &desc)) {
            std::cerr << "Warning: Failed to set service description: " << GetLastErrorAsString() << std::endl;
        }
    }

    // Set delayed auto-start if specified
    if (args.count("start") && args.at("start") == "delayed-auto") {
        SERVICE_DELAYED_AUTO_START_INFO delayedInfo = { TRUE };
        if (!ChangeServiceConfig2A(service, SERVICE_CONFIG_DELAYED_AUTO_START_INFO, &delayedInfo)) {
            std::cerr << "Warning: Failed to set delayed auto-start: " << GetLastErrorAsString() << std::endl;
        }
    }

    std::cout << "Service configuration updated successfully." << std::endl;

    // Clean up resources
    CloseServiceHandle(service);
    CloseServiceHandle(scManager);
    return true;
}

/**
 * Sets the failure actions for a Windows service
 * Similar to "sc failure <service> ..."
 *
 * @param serviceName Name of the service to configure
 * @param args Map of failure action parameters
 * @return true if successful, false otherwise
 */

bool SetServiceFailureActions(const std::string& serviceName, const std::map<std::string, std::string>& args) {
    // Open a handle to the service control manager with full access
    SC_HANDLE scManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (!scManager) {
        std::cerr << "Failed to open service control manager: " << GetLastErrorAsString() << std::endl;
        return false;
    }

    std::cout << "Attempting to configure service: '" << serviceName << "'" << std::endl;

    // Open a handle to the specified service with full access
    SC_HANDLE service = OpenServiceA(scManager, serviceName.c_str(), SERVICE_ALL_ACCESS);
    if (!service) {
        std::cerr << "Failed to open service: " << GetLastErrorAsString() << std::endl;
        CloseServiceHandle(scManager);
        return false;
    }

    // Initialize service failure actions structure with zeros
    SERVICE_FAILURE_ACTIONSA failureActions;
    ZeroMemory(&failureActions, sizeof(SERVICE_FAILURE_ACTIONSA));

    // ---------------- Parse Reset Period ----------------
    // Get and process reset period (default to 1 day/86400 seconds if not specified)
    int resetPeriod = 86400; // Default 1 day in seconds
    if (args.count("reset")) {
        try {
            std::string resetValue = args.at("reset");
            // Clean up the value (remove quotes, trim whitespace)
            resetValue.erase(std::remove(resetValue.begin(), resetValue.end(), '"'), resetValue.end());
            resetValue.erase(std::remove(resetValue.begin(), resetValue.end(), ' '), resetValue.end());

            resetPeriod = std::stoi(resetValue);
            std::cout << "Setting reset period to: " << resetPeriod << " seconds" << std::endl;
        }
        catch (const std::exception& e) {
            std::cerr << "Invalid reset period value, using default (86400): " << e.what() << std::endl;
        }
    }
    failureActions.dwResetPeriod = resetPeriod;

    // ---------------- Process Command and Reboot Message ----------------
    // These strings must remain in scope for the duration of the API call
    std::string command;
    std::string rebootMsg;

    // Set reboot message if provided
    if (args.count("reboot")) {
        rebootMsg = args.at("reboot");
        failureActions.lpRebootMsg = const_cast<LPSTR>(rebootMsg.c_str());
        std::cout << "Setting reboot message: '" << rebootMsg << "'" << std::endl;
    }

    // Set command to run on failure if provided
    if (args.count("command")) {
        command = args.at("command");
        failureActions.lpCommand = const_cast<LPSTR>(command.c_str());
        std::cout << "Setting command: '" << command << "'" << std::endl;
    }

    // ---------------- Process Actions ----------------
    // Container for our actions (will be copied to a fixed array for the API)
    std::vector<SC_ACTION> actionsVector;

    if (args.count("actions")) {
        std::string actionsStr = args.at("actions");
        std::cout << "Parsing actions string: '" << actionsStr << "'" << std::endl;

        // Remove quotes
        actionsStr.erase(std::remove(actionsStr.begin(), actionsStr.end(), '"'), actionsStr.end());

        // Split into action/delay pairs
        std::vector<std::string> pairs;
        size_t pos = 0;
        std::string delimiter = "/";

        while ((pos = actionsStr.find(delimiter)) != std::string::npos) {
            std::string token = actionsStr.substr(0, pos);
            token.erase(0, token.find_first_not_of(" \t"));
            token.erase(token.find_last_not_of(" \t") + 1);
            pairs.push_back(token);
            actionsStr.erase(0, pos + delimiter.length());
        }

        // Add the last part
        if (!actionsStr.empty()) {
            actionsStr.erase(0, actionsStr.find_first_not_of(" \t"));
            actionsStr.erase(actionsStr.find_last_not_of(" \t") + 1);
            pairs.push_back(actionsStr);
        }

        // Process pairs for action type and delay
        for (size_t i = 0; i < pairs.size() - 1; i += 2) {
            SC_ACTION action;
            ZeroMemory(&action, sizeof(SC_ACTION));

            // Process action type
            std::string actionType = pairs[i];
            if (actionType == "run") action.Type = SC_ACTION_RUN_COMMAND;
            else if (actionType == "restart") action.Type = SC_ACTION_RESTART;
            else if (actionType == "reboot") action.Type = SC_ACTION_REBOOT;
            else if (actionType == "none") action.Type = SC_ACTION_NONE;
            else {
                std::cerr << "Invalid action type: " << actionType << ", using 'none'" << std::endl;
                action.Type = SC_ACTION_NONE;
            }

            // Process delay
            if (i + 1 < pairs.size()) {
                try {
                    action.Delay = std::stoi(pairs[i + 1]) * 1000; // Convert seconds to milliseconds
                    std::cout << "Adding action: Type=" << action.Type
                        << ", Delay=" << action.Delay / 1000 << " seconds" << std::endl;
                }
                catch (const std::exception& e) {
                    std::cerr << "Invalid delay value: " << e.what() << ", using 0" << std::endl;
                    action.Delay = 0;
                }
            }

            actionsVector.push_back(action);
        }
    }

    // ---------------- Create fixed array for the API call ----------------
    SC_ACTION* actionsArray = nullptr;

    if (!actionsVector.empty()) {
        // Allocate the fixed array
        actionsArray = new SC_ACTION[actionsVector.size()];

        // Copy the actions
        for (size_t i = 0; i < actionsVector.size(); i++) {
            actionsArray[i] = actionsVector[i];
        }

        // Set up the failure actions structure
        failureActions.cActions = static_cast<DWORD>(actionsVector.size());
        failureActions.lpsaActions = actionsArray;
    }
    else {
        std::cout << "No actions specified or properly parsed" << std::endl;
        failureActions.cActions = 0;
        failureActions.lpsaActions = nullptr;
    }

    // ---------------- Make the API call ----------------
    BOOL result = ChangeServiceConfig2A(service, SERVICE_CONFIG_FAILURE_ACTIONS, &failureActions);

    // Clean up the actions array
    if (actionsArray) {
        delete[] actionsArray;
    }

    if (!result) {
        DWORD error = GetLastError();
        std::cerr << "Failed to set service failure actions. Error code: " << error
            << " - " << GetLastErrorAsString() << std::endl;

        CloseServiceHandle(service);
        CloseServiceHandle(scManager);
        return false;
    }

    // Verify the config was actually applied
    SERVICE_FAILURE_ACTIONSA verifyActions;
    ZeroMemory(&verifyActions, sizeof(verifyActions));
    DWORD bytesNeeded = 0;

    if (!QueryServiceConfig2A(service, SERVICE_CONFIG_FAILURE_ACTIONS,
        reinterpret_cast<LPBYTE>(&verifyActions),
        sizeof(verifyActions), &bytesNeeded)) {
        std::cerr << "Warning: Failed to verify configuration: " << GetLastErrorAsString() << std::endl;
    }
    else {
        std::cout << "Configuration verified:" << std::endl;
        std::cout << "  Reset Period: " << verifyActions.dwResetPeriod << " seconds" << std::endl;
        if (verifyActions.cActions > 0 && verifyActions.lpsaActions != nullptr) {
            for (DWORD i = 0; i < verifyActions.cActions; i++) {
                std::cout << "  Action " << i << ": Type=" << verifyActions.lpsaActions[i].Type
                    << ", Delay=" << verifyActions.lpsaActions[i].Delay << "ms" << std::endl;
            }
        }
    }

    std::cout << "Service failure actions configured successfully." << std::endl;

    // Clean up resources
    CloseServiceHandle(service);
    CloseServiceHandle(scManager);
    return true;
}


/**
 * Main entry point for the program
 * Parses command line arguments and dispatches to the appropriate command handler
 *
 * @param argc Number of command line arguments
 * @param argv Array of command line argument strings
 * @return 0 on success, 1 on failure
 */
int main(int argc, char* argv[]) {
    // Need at least a command
    if (argc < 2) {
        PrintUsage();
        return 1;
    }

    // Get the command (first argument)
    std::string command = argv[1];

    // Dispatch to appropriate command handler based on command name
    if (command == "query") {
        // Query service status
        if (argc < 3) {
            std::cerr << "ERROR: Service name required for query command." << std::endl;
            return 1;
        }
        return QueryService(argv[2]) ? 0 : 1;
    }
    else if (command == "create") {
        // Create a new service
        auto args = ParseArgs(argc, argv, 2);
        return CreateService(args) ? 0 : 1;
    }
    else if (command == "qdescription") {
        // Query service description
        if (argc < 3) {
            std::cerr << "ERROR: Service name required for qdescription command." << std::endl;
            return 1;
        }
        return QueryServiceDescription(argv[2]) ? 0 : 1;
    }
    else if (command == "start") {
        // Start a service
        if (argc < 3) {
            std::cerr << "ERROR: Service name required for start command." << std::endl;
            return 1;
        }
        return StartService(argv[2]) ? 0 : 1;
    }
    else if (command == "stop") {
        // Stop a service
        if (argc < 3) {
            std::cerr << "ERROR: Service name required for stop command." << std::endl;
            return 1;
        }
        return StopService(argv[2]) ? 0 : 1;
    }
    else if (command == "delete") {
        // Delete a service
        if (argc < 3) {
            std::cerr << "ERROR: Service name required for delete command." << std::endl;
            return 1;
        }
        return DeleteService(argv[2]) ? 0 : 1;
    }
    else if (command == "config") {
        // Configure a service
        if (argc < 3) {
            std::cerr << "ERROR: Service name required for config command." << std::endl;
            return 1;
        }
        auto args = ParseArgs(argc, argv, 3);
        return ConfigService(argv[2], args) ? 0 : 1;
    }
    else if (command == "failure") {
        // Set service failure actions
        if (argc < 3) {
            std::cerr << "ERROR: Service name required for failure command." << std::endl;
            return 1;
        }
        auto args = ParseArgs(argc, argv, 3);
        return SetServiceFailureActions(argv[2], args) ? 0 : 1;
    }
    else {
        // Unknown command
        std::cerr << "ERROR: Unknown command: " << command << std::endl;
        PrintUsage();
        return 1;
    }

    return 0;
}
