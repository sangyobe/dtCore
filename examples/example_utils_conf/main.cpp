#include <dtCore/src/dtUtils/dtConf.hpp>
#include <iostream>

static const char *yaml_1 = " \n\
type: leoquad                 \n\
name: LeoQuad                 \n\
version: v1.0                 \n\
dynamics:                     \n\
  mass: -0.03                 \n\
  com: [0.1, 0.0, 0.5]        \n\
";

static const char *yaml_2 = " \n\
version: v1.1                 \n\
dynamics:                     \n\
  com: [0.5, 0.0, 0.1]        \n\
";

int main(int argc, const char **argv)
{
    std::istringstream yaml_1_str(yaml_1);
    dt::Utils::Conf conf_1(yaml_1_str);
    std::cout << "---- yaml_1 ----" << std::endl;
    std::cout << "type: " << conf_1["type"].toString() << std::endl;
    std::cout << "name: " << conf_1["name"].toString() << std::endl;
    std::cout << "version: " << conf_1["version"].toString() << std::endl;
    std::cout << "mass: " << conf_1["dynamics"]["mass"].toDouble() << std::endl;
    std::cout << "COM: [ "
              << conf_1["dynamics"]["com"][0].toDouble() << ", "
              << conf_1["dynamics"]["com"][1].toDouble() << ", "
              << conf_1["dynamics"]["com"][2].toDouble() << " ]" << std::endl;
    conf_1.Dump("conf_1.yaml");

    std::istringstream yaml_2_str(yaml_2);
    dt::Utils::Conf conf_2(yaml_2_str);
    std::cout << "---- yaml_1 ----" << std::endl;
    std::cout << "version: " << conf_2["version"].toString() << std::endl;
    std::cout << "COM: [ "
              << conf_2["dynamics"]["com"][0].toDouble() << ", "
              << conf_2["dynamics"]["com"][1].toDouble() << ", "
              << conf_2["dynamics"]["com"][2].toDouble() << " ]" << std::endl;
    conf_2.Dump("conf_2.yaml");


    std::vector<std::shared_ptr<std::istream>> yaml_3_str;
    yaml_3_str.push_back(std::make_shared<std::istringstream>(yaml_1));
    yaml_3_str.push_back(std::make_shared<std::istringstream>(yaml_2));
    dt::Utils::Conf conf_3(yaml_3_str);
    conf_3.Dump("conf_3.yaml");
    std::cout << "---- yaml_1 + yaml_2 ----" << std::endl;
    std::cout << "type: " << conf_3["type"].toString() << std::endl;
    std::cout << "name: " << conf_3["name"].toString() << std::endl;
    std::cout << "version: " << conf_3["version"].toString() << std::endl;
    std::cout << "mass: " << conf_3["dynamics"]["mass"].toDouble() << std::endl;
    std::cout << "COM: [ "
              << conf_3["dynamics"]["com"][0].toDouble() << ", "
              << conf_3["dynamics"]["com"][1].toDouble() << ", "
              << conf_3["dynamics"]["com"][2].toDouble() << " ]" << std::endl;

    dt::Utils::Conf conf_1_f("conf_1.yaml");
    std::cout << "---- yaml_1 (from conf_1.yaml) ----" << std::endl;
    std::cout << "type: " << conf_1_f["type"].toString() << std::endl;
    std::cout << "name: " << conf_1_f["name"].toString() << std::endl;
    std::cout << "version: " << conf_1_f["version"].toString() << std::endl;
    std::cout << "mass: " << conf_1_f["dynamics"]["mass"].toDouble() << std::endl;
    std::cout << "COM: [ "
              << conf_1_f["dynamics"]["com"][0].toDouble() << ", "
              << conf_1_f["dynamics"]["com"][1].toDouble() << ", "
              << conf_1_f["dynamics"]["com"][2].toDouble() << " ]" << std::endl;

    dt::Utils::Conf conf_2_f("conf_2.yaml");
    std::cout << "---- yaml_2 (from conf_2.yaml) ----" << std::endl;
    std::cout << "version: " << conf_2_f["version"].toString() << std::endl;
    std::cout << "COM: [ "
              << conf_2_f["dynamics"]["com"][0].toDouble() << ", "
              << conf_2_f["dynamics"]["com"][1].toDouble() << ", "
              << conf_2_f["dynamics"]["com"][2].toDouble() << " ]" << std::endl;

    std::vector<std::string> yaml_3_f;
    yaml_3_f.push_back("conf_1.yaml");
    yaml_3_f.push_back("conf_2.yaml");
    dt::Utils::Conf conf_3_f(yaml_3_f);
    std::cout << "---- yaml_1 + yaml_2 (from conf_1.yaml, conf_2.yaml) ----" << std::endl;
    std::cout << "type: " << conf_3_f["type"].toString() << std::endl;
    std::cout << "name: " << conf_3_f["name"].toString() << std::endl;
    std::cout << "version: " << conf_3_f["version"].toString() << std::endl;
    std::cout << "mass: " << conf_3_f["dynamics"]["mass"].toDouble() << std::endl;
    std::cout << "COM: [ "
              << conf_3_f["dynamics"]["com"][0].toDouble() << ", "
              << conf_3_f["dynamics"]["com"][1].toDouble() << ", "
              << conf_3_f["dynamics"]["com"][2].toDouble() << " ]" << std::endl;

    return 0;
}