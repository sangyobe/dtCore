#include "dtConf.h"

namespace dtCore {

dtConf::dtConf(const std::string& yaml_file)
{
    _rootNode = YAML::LoadFile(yaml_file);
}

dtConf::dtConf(const YAML::Node& node)
{
    _rootNode = node;
}

const std::string dtConf::toString() const
{
    return _rootNode.as<std::string>();
}

const double dtConf::toDouble() const
{
    return _rootNode.as<double>();
}

const float dtConf::toFloat() const
{
    return _rootNode.as<float>();
}

const int32_t dtConf::toInt32() const
{
    return _rootNode.as<int32_t>();
}

const uint32_t dtConf::toUInt32() const
{
    return _rootNode.as<uint32_t>();
}

const bool dtConf::toBoolean() const
{
    return _rootNode.as<bool>();
}

#ifdef SYSREAL
const SYSREAL dtConf::toReal() const
{
    return _rootNode.as<SYSREAL>();
}
#endif

size_t dtConf::size()
{
    //assert(_rootNode.IsSequence());
    if (_rootNode.IsSequence())
        return _rootNode.size();
    else
        return 0; // Not an array !!!
}


}