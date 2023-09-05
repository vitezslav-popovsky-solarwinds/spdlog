#include <conio.h>
#include <atlstr.h>

#include "log_manager.h"
#include "service_worker.h"

#include <ShlObj.h>

std::string get_program_data_folder()
{
    PWSTR ppszPath = nullptr;
    if (SHGetKnownFolderPath(FOLDERID_ProgramData, KF_FLAG_DEFAULT, nullptr, &ppszPath) == S_OK)
    {
        std::string program_data(CW2A(ppszPath, CP_UTF8));
        CoTaskMemFree(ppszPath);
        return program_data;
    }

    return {};
}

int main()
{
    std::clog << "main function" << std::endl;
    const auto program_data = get_program_data_folder();
    if (program_data.empty())
    {
        std::cerr << "Fatal error: Unable to resolve ProgramData folder!" << std::endl;
        return 1;
    }

    const auto base_path = std::filesystem::path{ program_data } / "SpdLog";

    log_manager::instance().initialize(base_path);

    std::clog << "log_manager initialized" << std::endl;

    spdlog::info("Welcome to spdlog!");


    service_worker worker_foo("Foo");
    service_worker worker_bar("Bar");

    std::cout << "Press any key to continue. . .\n";
    _getch();

    worker_foo.stop();
    worker_bar.stop();
    worker_foo.join();
    worker_bar.join();

    log_manager::shutdown();

    return 0;
}

