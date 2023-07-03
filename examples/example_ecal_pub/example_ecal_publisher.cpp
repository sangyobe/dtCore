/*
 *
 * Copyright 2023 HMC ART developers.
 *
 */
#include <ecal/ecal.h>
#include <ecal/msg/protobuf/publisher.h>
#include <chrono>
#include <iostream>
#include <sys/time.h>
#include <dtProto/std_msgs/Heartbeat.pb.h>

#define PRINT_PUB_SUB_INFO (1)
void setTimestamp(google::protobuf::Timestamp *ts);

template <typename TPub, typename TMsg>
void PublishMessage(TPub &pub, TMsg &&msg) {
#if PRINT_PUB_SUB_INFO
  std::cout << "------------------------------------------" << std::endl;
  std::cout << "Publish topic : " << pub.GetTopicName() << std::endl;
  std::cout << "message type  : " << pub.GetTypeName() << std::endl;
  std::cout << "------------------------------------------" << std::endl;
  std::cout << "topic id      : " << msg.seq() << std::endl;
#endif
  std::chrono::system_clock::time_point start =
      std::chrono::system_clock::now();

  pub.Send(msg);

  std::chrono::system_clock::time_point end = std::chrono::system_clock::now();
  std::chrono::duration<double> sec = end - start;
#if PRINT_PUB_SUB_INFO
  std::cout << "------------------------------------------" << std::endl;
  std::cout << "elapsed(sec)  : " << sec.count() << std::endl;
  std::cout << "------------------------------------------" << std::endl;
#endif
}

int main(int argc, char **argv) {
  eCAL::Initialize(argc, argv, "dtProto HB pub");
  eCAL::Process::SetState(proc_sev_healthy, proc_sev_level1, "sub info");
  eCAL::protobuf::CPublisher<art_protocol::std_msgs::Heartbeat>
      publisher("dtProto_HB");
  
  auto cnt = 0;
  auto message(
      std::make_unique<art_protocol::std_msgs::Heartbeat>());

  while (eCAL::Ok()) {
    // fill message body
    message->set_seq(cnt++);
    setTimestamp(message->mutable_time_stamp());
    // publish a message
    PublishMessage(publisher, *message);
    eCAL::Process::SleepMS(500);
  }

  eCAL::Finalize();

  return 0;
}

void setTimestamp(google::protobuf::Timestamp *ts) {
#ifdef _WIN32
  FILETIME ft;
  GetSystemTimeAsFileTime(&ft);
  UINT64 ticks = (((UINT64)ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
  // A Windows tick is 100 nanoseconds. Windows epoch 1601-01-01T00:00:00Z
  // is 11644473600 seconds before Unix epoch 1970-01-01T00:00:00Z.
  ts->set_seconds((INT64)((ticks / 10000000) - 11644473600LL));
  ts->set_nanos((INT32)((ticks % 10000000) * 100));
#else
  struct timeval tv;
  gettimeofday(&tv, NULL);
  ts->set_seconds(tv.tv_sec);
  ts->set_nanos(tv.tv_usec * 1000);
#endif
}