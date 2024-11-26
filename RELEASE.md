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