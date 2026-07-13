// This file is part of dtCore, a C++ library for robotics software
// development.
//
// This library is commercial and cannot be redistributed, and/or modified
// WITHOUT ANY ALLOWANCE OR PERMISSION OF Hyundai Motor Company.

#ifndef __DT_DAQ_DATASINKPBMCAP_H__
#define __DT_DAQ_DATASINKPBMCAP_H__

#include <dtCore/src/dtDAQ/dtDataSinkPB.hpp>
#include <dtCore/src/dtUtils/dtFileHelper.h>
#include <dtCore/dtLog>

#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>
#include <string>
#include <queue>
#include <unordered_set>
#include <iostream>
#include <fstream>
#include <chrono>
#include <atomic>

#define MCAP_COMPRESSION_NO_LZ4
#define MCAP_COMPRESSION_NO_ZSTD
// #define MCAP_IMPLEMENTATION
#include <mcap/mcap.hpp>

namespace dt
{
namespace DAQ
{

/**
 * @brief Implements the IWritable interface used by McapWriter by wrapping a
 * FILE* pointer created by fopen().
 */
class MCAP_PUBLIC FileWriterCustom final : public mcap::IWritable {
public:
    ~FileWriterCustom() override
    {
        end();
    }

    int open(std::string filename, bool bTruncate)
    {
        if (bTruncate)
        {
            fd_ = ::open(filename.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
        }
        else
        {
            LOG(err) << "[MCAP] Failed to open file \"" << filename << "\" for writing: " << "support only truncate mode";
            return (-1);
        }

        if (fd_ < 0) {
            LOG(err) << "[MCAP] Failed to open file \"" << filename << "\" for writing: " << strerror(errno);
            return (-1);
        }
        size_ = 0;

        return 0;
    }

    void handleWrite(const std::byte* data, uint64_t size) override
    {
        assert(fd_ >= 0);
        size_t remaining = size;
        const std::byte* ptr = data;
        while (remaining > 0) 
        {
            ssize_t written = ::write(fd_, ptr, remaining);
            if (written < 0) 
            {
                if (errno == EINTR) 
                {
                    continue;
                }
                LOG(err) << "[MCAP] write() failed: " << strerror(errno);
                break;
            }
            ptr += written;
            remaining -= written;
            size_ += written;
        }
    }

    void end() override
    {
        if (fd_ >= 0) 
        {
            ::fsync(fd_);
            ::close(fd_);
            fd_ = -1;
        }

        size_ = 0;
    }

    uint64_t size() const override
    {
        return size_;
    }

private:
    int fd_ = -1;
    uint64_t size_ = 0;
};

template <typename T>
class DataSinkPBMcap : public DataSinkPB<T>
{
public:
    DataSinkPBMcap(
        const std::string &topic_name, 
        const std::string &file_basename = "", 
        bool annot_datetime = true, 
        bool truncate = true, 
        bool mcap_no_chunking = false, 
        std::size_t mcap_chunk_size = 1024 * 768)
        : _topic_name(topic_name)
    {
        if (annot_datetime)
            _file_name = dt::Utils::annotate_filename_datetime(file_basename);
        else
            _file_name = file_basename;

        std::string dirname;
        std::tie(dirname, std::ignore) = dt::Utils::split_by_directory(_file_name);
        if (!dirname.empty() && !dt::Utils::create_dir(dirname))
        {
            LOG(err) << "[MCAP] Failed to create containing directory (" << dirname << ").";
        }

        auto ret = _file_writer.open(_file_name, truncate);
        if (ret < 0)
        {
            return;
        }

        auto options = mcap::McapWriterOptions("");
        options.chunkSize = mcap_chunk_size;
        options.noChunking = mcap_no_chunking;
        options.compression = mcap::Compression::None;
        _writer.open(_file_writer, options);
        _is_open.store(true);

        // add schema
        mcap::Schema schema(
            T::descriptor()->full_name(),
            "protobuf", 
            BuildFileDescriptorSet(T::descriptor()).SerializeAsString());
        _writer.addSchema(schema);

        // add channel(topic)
        mcap::Channel channel(topic_name, "protobuf", schema.id);
        _writer.addChannel(channel);
        _channel_id = channel.id;
    }

    ~DataSinkPBMcap()
    {
        Close();
    }

    void Close()
    {
        if (!_is_open.load()) 
            return;
        _is_open.store(false);
        _writer.close();
        LOG(info) << "Closed MCAP file: " << _file_name;
    }

    void Publish(T& msg) override
    {
        if (!_is_open.load()) 
            return;

        mcap::Timestamp publishTime = std::chrono::duration_cast<std::chrono::nanoseconds>(
                                        std::chrono::system_clock::now().time_since_epoch()).count();

        std::string serialized = msg.SerializeAsString();
        mcap::Message mcap_msg;
        mcap_msg.channelId = _channel_id;
        mcap_msg.sequence = _msg_count++;
        mcap_msg.publishTime = publishTime;
        mcap_msg.logTime = publishTime;
        mcap_msg.data = reinterpret_cast<const std::byte*>(serialized.data());
        mcap_msg.dataSize = serialized.size();
        const auto res = _writer.write(mcap_msg);
        if (!res.ok()) {
            HandleWriteError();
        }
    }

    void Write(const mcap::Message &msg)
    {
        if (!_is_open.load()) 
            return;

        const auto res = _writer.write(msg);
        if (!res.ok())
        {
            HandleWriteError();
        }
    }

    bool IsOpen() const 
    {
        return _is_open.load(); 
    }

protected:
    google::protobuf::FileDescriptorSet BuildFileDescriptorSet(const google::protobuf::Descriptor* toplevelDescriptor) 
    {
        google::protobuf::FileDescriptorSet fdSet;
        std::queue<const google::protobuf::FileDescriptor*> toAdd;
        toAdd.push(toplevelDescriptor->file());
        std::unordered_set<std::string> seenDependencies;
        seenDependencies.insert(toplevelDescriptor->file()->name());
        while (!toAdd.empty()) 
        {
            const google::protobuf::FileDescriptor* next = toAdd.front();
            toAdd.pop();
            next->CopyTo(fdSet.add_file());
            for (int i = 0; i < next->dependency_count(); ++i) 
            {
                const auto& dep = next->dependency(i);
                if (seenDependencies.find(dep->name()) == seenDependencies.end()) 
                {
                    seenDependencies.insert(dep->name());
                    toAdd.push(dep);
                }
            }
        }
        return fdSet;
    }

private:
    void HandleWriteError()
    {
        _is_open.store(false);
        LOG(err) << "[MCAP] Failed to write message.\n";
        _writer.terminate();
        _writer.close();
    }

protected:
    std::string _topic_name;
    std::string _file_name;
    FileWriterCustom _file_writer;
    mcap::McapWriter _writer;
    mcap::ChannelId _channel_id{0};
    std::atomic<uint32_t> _msg_count{0};
    std::atomic<bool> _is_open{false};
};

} // namespace DAQ
} // namespace dt

#endif // __DT_DAQ_DATASINKPBMCAP_H__