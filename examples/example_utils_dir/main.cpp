#include <dtCore/src/dtUtils/dtDir.h>
#include <iostream>

int main(int argc, const char **argv)
{
    std::cout << "Executable Directory: " << dt::Utils::GetExecutableDir() << std::endl;
    std::cout << "Task Directory: " << dt::Utils::GetTasksDir() << std::endl;
    std::cout << "Model Path: " << dt::Utils::GetModelPath("hello.xml") << std::endl;
    std::cout << "File Extension: " << dt::Utils::GetFileExtension("hello.xml") << std::endl;
    std::cout << "Home Directory:: " << dt::Utils::GetUserHomeDir() << std::endl;

    return 0;
}