#include <dtCore/src/dtUtils/dtConf.hpp>
#include <iostream>

static const char *yaml = " \n\
type: leoquad               \n\
name: LeoQuad               \n\
version: v1.0               \n\
dynamics:                   \n\
  mass: -0.03               \n\
  com: [0.1, 0.0, 0.5]      \n\
";

int main(int argc, const char **argv)
{
    std::stringstream stream(yaml);
    dt::Utils::Conf conf(stream);
    std::cout << "type: " << conf["type"].toString() << std::endl;
    std::cout << "name: " << conf["name"].toString() << std::endl;
    std::cout << "version: " << conf["version"].toString() << std::endl;
    std::cout << "mass: " << conf["dynamics"]["mass"].toDouble() << std::endl;
    std::cout << "COM: [ "
              << conf["dynamics"]["com"][0].toDouble() << ", "
              << conf["dynamics"]["com"][1].toDouble() << ", "
              << conf["dynamics"]["com"][2].toDouble() << " ]" << std::endl;

    return 0;
}