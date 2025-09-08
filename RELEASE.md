#### [v1.12.0]
(2025/9/8)
##### 
- dt::Utils::Conf accepts list of files or istream objects and merge them to construct a new Conf instance.

#### [v1.11.1]
(2025/8/25)
##### 
- add dtproto.geometry_msgs.Transform message definition

#### [v1.11.0]
(2025/8/19)
##### 
- add width parameter to dtproto.geometry_msgs.Path
- add robot_msgs.RobotCommandResponse message definition
- RobotCommand service returns with robot_msgs.RobotCommandResponse instead of std_msgs.Response
  
#### [v1.10.0]
(2025/7/18)
##### 
- add dtproto.nav_msgs.NavCommand
- add dtproto.nav_msgs.NavState
- add dtProto.geometry_msgs.Path
- add path field on dtProto.robot_msgs.VisualizeState

#### [v1.9.1]
(2025/7/1)
##### 
- dtproto.dtService.RobotCommand 서비스 추가(unary rpc)
- dtproto.dtService.SubscribeRobotCommand 서비스 추가(client-side streaming rpc)
- dtproto.robot_msgs.RobotCommand 구조체 정의 변경(ControlCmd, JointControl, MoveControl 옵션 추가)
- dtConf 의 size() 멥머 리턴 타입을 const로 변경

#### [v1.9.0]
(2025/5/27)
##### 
- dtproto.robot_msgs.RobotCommand is added.
- dtproto.geometry_msgs.SE2Trajectory/SE3Trajectory are added.
- dtproto.geometry_msgs.SE2Velocity/SE3Velocity are added.
- dtproto.geometry_msgs.SE2Pose/SE3Pose are added for usability.
- dtproto.geometry_msgs.Vec2x/Vec3x are added for usability.

#### [v1.8.1]
(2025/3/24)
##### 
- The dtConf outputs an error message for non-existing nodes when a YAML::InvalidNode exception is issued.

#### [v1.8.0]
(2025/3/24)
##### 
- dtproto.dtService.StreamJoy() service prototype has been changed to support bi-directional data communication including timestamp.
- dtproto.dtService.StreamJoy() service returns dtproto.sensor_msgs.Joy instead of dtproto.std_msgs.Response.
- dtCore header files are added to MSVC project.

#### [v1.7.2]
(2025/2/5)
##### 
- dt::DAQ::ServiceListenerGrpc::AddSession() returns session id created.
- dt::DAQ::ServiceListenerGrpc::GetSession() is added.
- dt::DAQ::ServiceListenerGrpc::Session::Send() is added.
- dt::DAQ::ServiceCallerGrpc::Call::Send() is added.

#### [v1.7.1]
(2025/1/23)
##### 
- add DataSinkPBMcap::Write() interface to copy a raw mcap message.
- add dtTimeUtil.hpp implementing time manipulation utilities.

#### [v1.7.0]
(2025/1/20)
##### 
- add DataSinkPBMcapRotator to support rotating filename based on file maximum size.
 
#### [v1.6.1]
(2024/12/18)
##### 
- reset ServiceCallerGrpc::_running flag as false when message queue thread exits.

#### [v1.6.0]
(2024/12/13)
##### 
- perception_msgs added.
- dtproto.perception service added.
- dt::Utils::Conf supports load(save) from(to) a file
 
#### [v1.5.2]
(2024/11/26)
##### 
- Conf(const char *yaml_str) -> Conf(const std::istream &yaml_str)

#### [v1.5.1]
(2024/11/25)
##### 
- add cmake config to support find_package().
- dt::Utils::dtConf gets character string as constructor argument.
- add dt::Utils::Watchdog.

#### [v1.5.0]
(2024/8/27)
##### 
- dtproto.nav_msgs.Grid 데이터 정의 변경 (multi-layer gridmap 데이터 전송을 위해 데이터 구조 확장)
 
#### [v1.4.0]
(2024/7/17)
##### 
- 'dt' namespace 적용
- dtTrajectory를 별도의 프로젝트로 분리
- dtMath 의존성 제거(GnuPlot)

#### [v1.3.1]
(2024/7/2)
##### dtProto
- dtproto.robot_msgs.VisualizeState 메시지 정의 추가
- dtproto.geometry_msgs.Marker 메시지 정의 추가
- dtproto.nav_msgs.Grid 메시지 정의 업데이트 : grid_center 추가

#### [v1.3.0]
(2024/6/28)
##### dtProto
- RPC Service name convention 적용 : Request/Publish/Stream/Command

##### dtDAQ
- ServiceCallerGrpc::GetCall() 인터페애스 추가 : client-side streaming 지원

#### [v1.2.2]
(2024/6/5)
##### dtDir
- 유저 home 디렉토리 구하는 함수(GetUserHomeDir()) 추가

#### [v1.2.1]
(2024/6/5)
##### ServiceListenerGrpc
- Stop() 호출시 연결된 session에 대한 call을 즉시 cancel함.
- FINISH 상태의 session에 대해 compeletion queue event 처리하지 않음(중복된 Finish() 호출 오류 fix)

#### [v1.2.0]
(2024/5/13)
- dtcore, dtcore_grpc 라이브러리(cmake target) 추가
- dtThread, dtUtils 헤더 및 소스코드 추가