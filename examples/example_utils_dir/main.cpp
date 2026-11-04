#include <dtCore/src/dtUtils/dtDir.h>
#include <iostream>

int main(int argc, const char **argv)
{
    std::cout << " ** working directories ** " << std::endl;
    std::string cwd = dt::Utils::GetCurrentWorkingDir();
    std::cout << "Current Working Directory: " << cwd << std::endl;
    
    std::cout << " set the current working directory to the executable directory... " << std::endl;
    dt::Utils::SetCurrentWorkingDir(dt::Utils::GetExecutableDir());
    std::cout << "Current Working Directory: " << dt::Utils::GetCurrentWorkingDir() << std::endl;
    
    std::cout << " restore the current working directory... " << std::endl;
    dt::Utils::SetCurrentWorkingDir(cwd);
    std::cout << "Current Working Directory: " << dt::Utils::GetCurrentWorkingDir() << std::endl;

    std::cout << " ** miscellaneous ** " << std::endl;
    std::cout << "Home Directory:: " << dt::Utils::GetUserHomeDir() << std::endl;
    std::cout << "Executable Directory: " << dt::Utils::GetExecutableDir() << std::endl;
    std::cout << "Task Directory: " << dt::Utils::GetTaskDir() << std::endl;
    std::cout << "Model Directory: " << dt::Utils::GetModelDir() << std::endl;
    std::cout << "Config Directory: " << dt::Utils::GetConfigDir() << std::endl;
    std::cout << "File Extension: " << dt::Utils::GetFileExtension("hello.xml") << std::endl;
    
    std::cout << " ** deprecated ** " << std::endl;
    std::cout << "Model Path: " << dt::Utils::GetModelPath("hello.xml") << std::endl;
    
   return 0;
}