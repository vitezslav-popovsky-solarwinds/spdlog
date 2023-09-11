#include <conio.h>
#include <atlstr.h>
#include <ShlObj.h>

#include "log_manager.h"
#include "service_worker.h"

std::string expand_environment_strings(const std::string& src)
{
    _TCHAR dest_buffer[MAX_PATH];
    ExpandEnvironmentStrings(CA2W(src.c_str(), CP_UTF8).m_psz, dest_buffer, ARRAYSIZE(dest_buffer));
    return {CW2A(dest_buffer, CP_UTF8).m_szBuffer};
}

int main()
{
    std::clog << "main function" << std::endl;

    std::string base_log_path(R"(%ALLUSERSPROFILE%\SpdLog)");
    base_log_path = expand_environment_strings(base_log_path);

    std::clog << "expanded log directory path: " << base_log_path << std::endl;

    if(!log_manager::instance().initialize(std::filesystem::path{ base_log_path }))
    {
        return 1;
    }

    std::clog << "log_manager initialized" << std::endl;

    service_worker worker_foo("Foo");
    service_worker worker_bar("Bar");

    std::cout << "Press any key to continue. . ." << std::endl;
    _getch();

    worker_foo.stop();
    worker_bar.stop();
    worker_foo.join();
    worker_bar.join();

    log_manager::shutdown();

    return 0;
}

