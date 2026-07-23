#### [v1.16.1]
(2026/6/15)
##### dt::Log (dtRtLog)
- (Hot fix) Create()로 생성한 로거의 flush() 호출 누락 수정
- Logger flush 주기 변경: 100ms (10Hz) -> 20ms (50Hz)

#### [v1.16.0]
(2026/7/23)
##### dt::Log (dtRtLog)
- `LOG_U(logger_name, level)` 매크로 추가: `Create()`로 생성한 named logger에 RT-safe 스트림 로그 기록 (`NamedLogRtStream`)
- `LOG_CONT(level)` 매크로 추가: prefix 없이 연속 출력을 위한 continuation 로그 스트림 (`LogRtContStream`). 명시적 `\n`으로 줄 바꿈, 21칸 자동 indent 적용
- `Create(logName, fileBasename, ...)` 함수 추가: named logger 생성 및 spdlog 레지스트리 등록. `"_STDOUT_"` 지정 시 stdout 출력, 그 외 파일 출력
- `SetLogLevel(logger_name, lvl)` / `SetLogPattern(logger_name, ...)` / `FlushOn(logger_name, lvl)` 오버로드 추가: named logger별 레벨·패턴 개별 설정 지원
- `SetLogPattern()` raw spdlog 패턴 문자열 직접 지정 오버로드 추가
- `Sync()` 함수 추가: 드레인 스레드가 현재 큐의 모든 메시지를 처리할 때까지 대기 (non-RT context 전용)
- `ColorStdoutSinkT` 커스텀 sink 추가: 64KB 내부 버퍼 + O_NONBLOCK 단일 `write()`로 Xenomai primary mode 유지
- `BasicFileSinkT` 커스텀 file sink 추가: 64KB 내부 버퍼 + 파일 크기 기반 rotation 지원
- 드레인 스레드 적응형 polling 간격 적용: 큐 점유율에 따라 100μs ~ 1ms 자동 조정
- 드레인 스레드 기본 CPU core #2 변경 (`THREAD_CPU_ID = 2`)
- `config.yaml`의 `logFile` 경로에 디렉토리가 없을 경우 자동 생성
- `example_rtlog_pattern`, `example_rtlog_create` 예제 추가
##### dt::Thread
- `CreateRtThread()` / `CreateNonRtThread()` 통합: `CreateThread(thread, realtime, addList)` 단일 함수로 API 일원화
- 스레드 생성 진단 출력을 `dtTerm::Printf` → `LOG_CONT(info)`로 변경
- `pthread_attr_setschedparam()` 누락 및 `CPU_SETSIZE` → `sizeof(cpuset)` 오류 수정
##### dt::DAQ (dtDataSinkPBMcap)
- MCAP 파일 손상 문제 수정 (ARTF-7): `handleWrite()` 내 partial write 재시도 루프 추가, `FileWriterCustom` 클래스로 직접 syscall `write()` 사용
- append 모드 지원 추가: truncate 없이 파일 열기 가능. 기존 파일 크기를 `size_`에 반영하여 McapWriter 오프셋 계산 정확성 확보
##### dtProto
- `dtproto::sensor_msgs::Image` 메시지 정의 추가

#### [v1.15.0]
(2026/6/15)
##### dt::Log (dtRtLog / dtRtTui) — 신규 모듈
- `dt::Log::RtLog` 신규 도입: Xenomai 등 실시간 시스템을 위한 RT-safe 로거
  - MPSC lock-free 큐(1024슬롯 × 1024B/msg) 기반, 드레인 스레드에서 비동기 출력
  - `LOG(level)` 매크로: `LogRtStream` 스트림 인터페이스 (`operator<<`, `printf()`, `format()`)
  - `LOG_RT_RAW(level, fmt, ...)` 매크로: STDERR 직접 출력 (드레인 스레드 미사용, 신호 핸들러·긴급 경로 전용)
  - `std::hex`, `std::dec`, `std::setw`, `std::setfill` 스트림 조작자 지원
  - Eigen `MatrixBase` 및 dt::Math `Vector` 계열 타입 `operator<<` 지원
  - `std::vector<T>` 배열 형식(`[v0, v1, ...]`) 출력 지원
  - 이전 `dt::Log` 클래스 대비 마이그레이션 편의를 위한 `using Log = RtLog` alias 추가
- `dt::Log::RtTui` 신규 도입: 실시간 데이터 시각화를 위한 TUI 모드
  - Area 1: 그룹/행 구조의 수치 데이터 표시 (`TUI_SET_GROUP`, `TUI_SET_ROW`, `TUI_SET_ROW_COLS`, `TUI_SET_TEXT_ROW`, `TUI_SET_TEXT_ROW_FMT`)
  - Area 2: 컬러 레벨 표시 로그 출력 영역
  - 멀티 레이아웃 지원: `[` / `]` 키로 레이아웃 전환, `TUI_SET_LAYOUT_NAME`으로 레이아웃명 설정
  - `TUI_GET_PENDING_KEY()`: 키 입력 non-blocking 조회
  - `TUI_COL(fmt, val)`: 컬럼별 포맷 개별 지정
- `Initialize()` 내부에서 드레인 스레드 자동 생성으로 변경 (외부 생성 방식 제거)
- `example_log_tui` 예제 추가
##### dt::Thread
- `CreateRtThread()` / `CreateNonRtThread()`에 `pthread_setname_np()` 추가: OS 스레드 이름 설정

#### [v1.14.0]
(2026/3/16)
##### 
- dt::Utils::join_path 함수 추가: path 와 file명 concatenation(join).
  std::string join_path(const std::string &pathname, const std::string &filename);
- MCAP_IMPLEMENTATION 정의를 dtDataSinkPBMcap.hpp에서 제외: 동시에 두개의 소스에서 해당 파일 include 시 link 에러 발생하는 현상 fix. dtDataSinkPBMcap.hpp 등을 이용해서 mcap 라이브러리 사용할 경우 어플리케이션 소스에서 반드시 dtCore/src/dtDAQ/mcapImp.hpp를 한번 그리고 오직 한번 인크루드 해줘야 함.

#### [v1.13.4]
(2026/2/24)
##### 
- dt::Log에 syslog 기능 추가(linux, macOS 등)
- dt::Log::FlushEvery() 함수 추가: 주기적인 flush 기능 설정
- dt::Log::SetLogOffAll() 함수 추가
- dt::Log::FlushOnAll() 함수 추가
- dt::Log::SetLogLevelAll() 함수 추가
- dt::Log::SetLogPatternAll() 함수 추가
- dt::Log::SetLogPatternXXX() 함수가 spdlog log pattern string을 직접 입력 받을 수 있도록 함수 overload
- examples에 log 관련 기능 테스트를 위한 샘플 코드/프로젝트 추가
- dt::DAQ::DataSinkPBMcap 생성자에 mcap_no_chunking:bool, mcap_chunk_size:std::size_t 아규먼트 추가하여 mcap 저장시 chunking 옵션 및 chunk size 설정 가능하도록 변경
- dt::Utils::Conf 에 toFloat64/toInt8/toUInt8/toInt16/toUInt16/toInt64/toUInt64 함수 추가
  
#### [v1.13.3]
(2026/1/30)
##### 
- Add dt::Utils::Lock
  
#### [v1.13.2]
(2025/12/8)
##### 
- [Bug fix] Make 'std::ostream& dt::Utils::operator<<(std::ostream& out, const Conf& conf);' function inline.
- Add a new data field, raw, for holding sensor raw data in dtproto.sensor_msgs.Ft message.

#### [v1.13.1]
(2025/12/2)
##### 
- The dt::Utils::Conf module allows you to add new configuration items or change their values. 
  Additionally, the ability to save configuration contents to a file has been added.
- The example to test The dt::Utils::Conf module has been updated.
- add dtproto::sensor_msgs::Ft, dtproto::sensor_msgs::FtTimeStamped.

#### [v1.13.0]
(2025/9/25)
##### 
- extend dtproto::perception_msgs::Object to include the size and type of the object.
- add dtproto::geometry_msgs::Size.

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