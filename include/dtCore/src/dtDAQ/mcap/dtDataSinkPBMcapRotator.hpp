// This file is part of dtCore, a C++ library for robotics software
// development.
//
// This library is commercial and cannot be redistributed, and/or modified
// WITHOUT ANY ALLOWANCE OR PERMISSION OF Hyundai Motor Company.

#ifndef __DT_DAQ_DATASINKPBMCAPROTATOR_H__
#define __DT_DAQ_DATASINKPBMCAPROTATOR_H__

#include "dtDataSinkPBMcap.hpp"

namespace dt
{
namespace DAQ
{

template <typename T>
class DataSinkPBMcapRotator : public DataSinkPB<T>
{
public:
    DataSinkPBMcapRotator(const std::string &topic_name, const std::string &file_basename = "", bool annot_datetime = true, bool truncate = true, std::size_t max_file_size = 1048576, uint32_t max_file_count = 10)
        : _topic_name(topic_name), _max_file_size(max_file_size), _max_file_count(max_file_count), _current_file_size(0)
    {
        if (annot_datetime)
            _file_name = dt::Utils::annotate_filename_datetime(file_basename);
        else
            _file_name = file_basename;

        std::string dirname;
        std::tie(dirname, std::ignore) = dt::Utils::split_by_directory(_file_name);

        if (!dirname.empty() && !dt::Utils::create_dir(dirname))
        {
            std::cerr << "Failed to create containing directory (" << dirname << ")." << std::endl;
        }

        // create the 1st mcap file
        _current_mcap = std::make_unique<DataSinkPBMcap<T>>(topic_name, _file_name, false);
    }

    void Publish(T &msg)
    {
        if (!_current_mcap)
            return;

        std::size_t expected_msg_size = msg.ByteSizeLong();
        std::size_t new_file_size = _current_file_size + expected_msg_size;

        if (new_file_size > _max_file_size)
        {
            // rotate mcap files
            Rotate();
            //reset current file size
            new_file_size = expected_msg_size;
        }

        _current_mcap->Publish(msg);
        _current_file_size = new_file_size;
    }

    void Write(const mcap::Message &msg)
    {
        if (!_current_mcap)
            return;

        std::size_t expected_msg_size = msg.dataSize;
        std::size_t new_file_size = _current_file_size + expected_msg_size;

        if (new_file_size > _max_file_size)
        {
            // rotate mcap files
            Rotate();
            //reset current file size
            new_file_size = expected_msg_size;
        }

        _current_mcap->Write(msg);
        _current_file_size = new_file_size;
    }

protected:
    void Rotate()
    {
        // close current mcap file
        if (_current_mcap)
            _current_mcap.reset();

        // rotate mcap files
        for (auto i = _max_file_count; i > 0; --i)
        {
            std::string source_filename = dt::Utils::annotate_filename_index(_file_name, i - 1);
            if (!dt::Utils::path_exists(source_filename))
            {
                continue;
            }
            std::string target_filename = dt::Utils::annotate_filename_index(_file_name, i);
            if (!dt::Utils::rename_file(source_filename, target_filename))
            {
                // truncate the old mcap file and create a new one
                _current_mcap = std::make_unique<DataSinkPBMcap<T>>(_topic_name, _file_name, false, true);
                throw("rotating_file_sink: failed renaming " + source_filename + " to " + target_filename, errno);
            }
        }

        // create a new mcap file
        _current_mcap = std::make_unique<DataSinkPBMcap<T>>(_topic_name, _file_name, false, true);
    }

protected:
    std::unique_ptr<DataSinkPBMcap<T>> _current_mcap{nullptr};
    std::string _topic_name;
    std::string _file_name;
    std::size_t _max_file_size;
    uint32_t _max_file_count;
    std::size_t _current_file_size; // estimated file size
};

} // namespace DAQ
} // namespace dt

#endif // __DT_DAQ_DATASINKPBMCAPROTATOR_H__