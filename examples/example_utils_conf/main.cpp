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
    //
    // generate conf_1.yaml from stringstream
    //
    std::istringstream yaml_1_str(yaml_1);
    dt::Utils::Conf conf_1(yaml_1_str);
    std::cout << "---- yaml_1 ----" << std::endl;
    std::cout << conf_1 << std::endl;
    conf_1.Dump("conf_1.yaml");

    //
    // generate conf_2.yaml from stringstream
    //
    std::istringstream yaml_2_str(yaml_2);
    dt::Utils::Conf conf_2(yaml_2_str);
    std::cout << "---- yaml_2 ----" << std::endl;
    std::cout << conf_2 << std::endl;
    conf_2.Dump("conf_2.yaml");

    //
    // generate conf_3.yaml from list of stringstreams
    //
    std::vector<std::shared_ptr<std::istream>> yaml_3_str;
    yaml_3_str.push_back(std::make_shared<std::istringstream>(yaml_1));
    yaml_3_str.push_back(std::make_shared<std::istringstream>(yaml_2));
    dt::Utils::Conf conf_3(yaml_3_str);
    std::cout << "---- yaml_1 + yaml_2 ----" << std::endl;
    std::cout << conf_3 << std::endl;
    conf_3.Dump("conf_3.yaml");

    //
    // load conf_1.yaml
    //
    dt::Utils::Conf conf_1_f("conf_1.yaml");
    std::cout << "---- yaml_1 (from conf_1.yaml) ----" << std::endl;
    std::cout << conf_1_f << std::endl;

    //
    // load conf_2.yaml
    //
    dt::Utils::Conf conf_2_f("conf_2.yaml");
    std::cout << "---- yaml_2 (from conf_2.yaml) ----" << std::endl;
    std::cout << conf_2_f << std::endl;

    //
    // load list of yaml files, conf_1.yaml and conf_2.yaml
    //
    std::vector<std::string> yaml_3_f;
    yaml_3_f.push_back("conf_1.yaml");
    yaml_3_f.push_back("conf_2.yaml");
    dt::Utils::Conf conf_3_f(yaml_3_f);
    std::cout << "---- conf_1.yaml + conf_2.yaml ----" << std::endl;
    // std::cout << "type: " << conf_3_f["type"].toString() << std::endl;
    // std::cout << "name: " << conf_3_f["name"].toString() << std::endl;
    // std::cout << "version: " << conf_3_f["version"].toString() << std::endl;
    // std::cout << "mass: " << conf_3_f["dynamics"]["mass"].toDouble() << std::endl;
    // std::cout << "COM: [ "
    //           << conf_3_f["dynamics"]["com"][0].toDouble() << ", "
    //           << conf_3_f["dynamics"]["com"][1].toDouble() << ", "
    //           << conf_3_f["dynamics"]["com"][2].toDouble() << " ]" << std::endl;
    std::cout << conf_3_f << std::endl;

    //
    // test accessors
    //
    std::cout << "---- conf_1.yaml + conf_2.yaml (accessors) ----" << std::endl;
    std::cout << "type: " << conf_3_f["type"].toString() << std::endl;
    std::cout << "name: " << conf_3_f["name"].toString() << std::endl;
    std::cout << "version: " << conf_3_f["version"].toString() << std::endl;
    std::cout << "mass: " << conf_3_f["dynamics"]["mass"].toDouble() << std::endl;
    std::cout << "dynamics:" << std::endl
              << "  com: [ "
              << conf_3_f["dynamics"]["com"][0].toDouble() << ", "
              << conf_3_f["dynamics"]["com"][1].toDouble() << ", "
              << conf_3_f["dynamics"]["com"][2].toDouble() << " ]" << std::endl;

    //
    // modify(assign) parameter values and add a new config parameter to conf_3 and save as conf_4.yaml
    //
    dt::Utils::Conf conf_4 = conf_3;
    conf_4["version"] = "v1.2";
    std::vector<double> Kp_init = {0.1, 0.2, 0.5};
    conf_4["gain"]["Kp"] = Kp_init;
    std::cout << "---- conf_4 (add new parameter, gain.Kp) ----" << std::endl;
    std::cout << "gain:" << std::endl
              << "  Kp :" << std::endl
              << conf_4["gain"]["Kp"] << std::endl;

    conf_4["gain"]["Kp"][0] = 1.5;
    conf_4["gain"]["Kp"][1] = 2.5;
    conf_4["gain"]["Kp"][2] = 3.5;
    std::cout << "---- conf_4 (modify parameter, gain.Kp) ----" << std::endl;
    std::cout << "gain:" << std::endl
              << "  Kp :" << std::endl
              << conf_4["gain"]["Kp"] << std::endl;

    conf_4["gain"]["Kp"] << 11.5 << 12.5 << 13.5;
    std::cout << "---- conf_4 (append values to gain.Kp) ----" << std::endl;
    std::cout << "gain:" << std::endl
              << "  Kp :" << std::endl
              << conf_4["gain"]["Kp"] << std::endl;

    std::cout << "---- conf_4 (final) ----" << std::endl;
    std::cout << conf_4 << std::endl;
    conf_4.Dump("conf_4.yaml");

    return 0;
}