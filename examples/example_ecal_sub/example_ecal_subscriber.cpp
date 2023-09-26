/*
 *
 * Copyright 2023 HMC ART developers.
 *
 */
#include <ecal/ecal.h>
#include <ecal/msg/protobuf/subscriber.h>
#include <chrono>
#include <iostream>
#include <sys/time.h>
#include <dtProto/std_msgs/Heartbeat.pb.h>

#define PRINT_PUB_SUB_INFO

void OnHeartbeat(const char *topic_name,
                const dtproto::std_msgs::Heartbeat &message,
                const long long time, const long long clock) {
#ifdef PRINT_PUB_SUB_INFO
  std::cout << "------------------------------------------" << std::endl;
  std::cout << " Hearbeat "                                 << std::endl;
  std::cout << "------------------------------------------" << std::endl;
  std::cout << "topic name   : " << topic_name              << std::endl;
  std::cout << "topic time   : " << time                    << std::endl;
  std::cout << "topic clock  : " << clock                   << std::endl;
  std::cout << "------------------------------------------" << std::endl;
  std::cout << " Header "                                   << std::endl;
  std::cout << "------------------------------------------" << std::endl;
  std::cout << "  seq        : " << message.seq()           << std::endl;
  std::cout << "------------------------------------------" << std::endl;
  std::cout                                                 << std::endl;
#endif // PRINT_PUB_SUB_INFO
}

int main(int argc, char** argv)
{
  eCAL::Initialize(argc, argv, "dtProto HB sub");
  eCAL::Process::SetState(proc_sev_healthy, proc_sev_level1, "sub info");
  eCAL::protobuf::CSubscriber<dtproto::std_msgs::Heartbeat>
      subscriber("dtProto_HB");
  auto callback_heartbeat = std::bind(OnHeartbeat, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
  subscriber.AddReceiveCallback(callback_heartbeat);

  while (eCAL::Ok())
  {
    eCAL::Process::SleepMS(100);
  }

  eCAL::Finalize();

  return 0;
}