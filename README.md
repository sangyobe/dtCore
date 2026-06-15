# dtCore
**dtCore is a ART Foundation C++ (template) library. It includes basic data structures, network messaging framework, file parsers, data logger, etc. ART stands for Articulated Robotics Team**

## Description

### dtProto
* Protocol Buffer 기반으로 C++ 메시지 정의 및 serialization을 지원합니다. 
* Protocol buffer SDK 및 컴파일러(protoc)가 설치되어 있어야 합니다. Protocol buffer 설치시 cmake 연동을 위한 module config 파일들도 설치되어야 합니다.
* 개발시 protoc 3.21.12 버전에서 개발 및 테스트되었습니다.
* 유저코드에서 기능을 사용하기 위해서는 헤더파일(/include/dtProto/*.h)과 빌드된 라이브러리(/lib/libdtproto.a, libdtproto_grpc.a)가 필요합니다.
* [TODO] Transport layer로 gRPC, eCAL을 고려 중이며 현재 테스트 중입니다.
* [TODO] eCAL 의 경우 protobuf 동적링크(shared) 라이브러리를 사용합니다. 이에 관한 빌드 옵션 및 라이브러리 버전간 충돌 문제가 발생할 수 있어 테스트 중입니다.

### dtLog
* Terminal 및 Local file 로 이벤트 로그 저장을 지원합니다.
* std::format 스타일의 로그 메시지 저장을 지원합니다. 
* streaming operator(<<) 스타일의 로그 메시지 저장을 지원합니다.
* 로그 레벨(error, warning, info, debug 등) 지정을 지원합니다.
#### dtLog / RtLog
* RT 태스크에서도 로그 기능을 이용할 수 있습니다.
* printf 스타일의 로그 메시지 저장을 지원합니다.
* dtTerm의 기능을 하나로 통합하였습니다.
#### dtLog -> RtLog Migration 방법
* 아래와 같이 헤더파일 인클루드 변경, Initialize() 함수 인자 변경, 기존 dtTerm::Printf 함수 변경 작업 필요
```
#include <dtCore/dtLog>     // <-- dtCore/src/dtLog/dtLog.h & dtCore/dtUtils/dtTerminal.h

void dt::Log::Initialize(
    const std::string &logName,                             // Logger name
    const std::string &fileBasename = "",                   // Log file base name
    bool enableTui = false,                                 // TUI mode: true(enable) / false(disable)
    int threadCpuId = RtLogConstant::THREAD_CPU_ID,         // default cpu core: core 1
    int threadPriority = RtLogConstant::THREAD_PRIORITY,    // default priority: 0 (non-RT)
    size_t threadStack = RtLogConstant::THREAD_STACK_SIZE,  // default stack: 1MB
    size_t maxFiles = RtLogConstant::DEFAULT_MAX_FILES,     // max log file number: 5
    size_t maxFileSize = RtLogConstant::DEFAULT_MAX_SIZE,   // max log file size: 10MB
    bool annotDatetime = true,
    bool truncate = false);

LOG(level).printf(...)      // <-- dtTerm::Printf(...)

// LOG(level) << ... 
// 위와 같은 스트림 방식으로 호출할 경우 에러 발생시, 
// LOG(level).printf 또는 LOG(level).format 과 같은 가변인자 방식으로 사용
```

* TUI Migration: 기존 dtTerm::Printf로 작업하는 대신, 아래와 같은 매크로들을 이용해서 TUI 화면을 구성하도록 변경 필요

| 매크로 | 파라미터 | 예시 | 비고 |
| :---  | :---    | :---  | :--- |
| TUI_SET_LAYOUT_NAME | layout 번호(int), layout 라벨(str) | TUI_SET_LAYOUT_NAME(0, "Layout#1") | layout은 최대 9개까지 추가 가능 |
| TUI_SET_GROUP | layout 번호(int), group 번호(int), group 라벨(str), column 라벨(empty or string array) | TUI_SET_GROUP(0, 0, "Joint State", "pos", "vel", "tor") | layout 당 group은 최대 10개까지 설정 가능<br>group 당 column은 최대 10개까지 설정 가능 |
| TUI_SET_GROUP_NO_HDR | layout 번호(int), group 번호(int) | TUI_SET_GROUP_NO_HDR(0, 0) |	headerless group 추가 |
| TUI_SET_TEXT_ROW_FMT | layout 번호(int), group 번호(int), row 번호(int), row 라벨(str), 데이터(printf 방식) | TUI_SET_TEXT_ROW_FMT(0, 0, 0, "Ctrl",<br>&nbsp;&nbsp;&nbsp;"period: %6.3f ms, load: %6.3f ms, maxLoad: %6.3f ms, overrun: %d",<br>&nbsp;&nbsp;&nbsp;sysData->ctrlTime.period_ms,<br>&nbsp;&nbsp;&nbsp;sysData->ctrlTime.algo_ms,<br>&nbsp;&nbsp;&nbsp;sysData->ctrlTime.algoMax_ms,<br>&nbsp;&nbsp;&nbsp;sysData->ctrlTime.overrun) | group 당 row는 최대 20개까지 설정 가능<br>string 타입 출력 |
| TUI_SET_ROW | layout 번호(int), group 번호(int), row 번호(int), row 라벨(str), 데이터 포맷(printf 방식), 데이터 (array) | TUI_SET_ROW(0, 0, 0, "Test Int", "%d",<br>&nbsp;&nbsp;&nbsp;random_int_list[0],<br>&nbsp;&nbsp;&nbsp;random_int_list[1],<br>&nbsp;&nbsp;&nbsp;random_int_list[2],<br>&nbsp;&nbsp;&nbsp;random_int_list[3],<br>&nbsp;&nbsp;&nbsp;random_int_list[4],<br>&nbsp;&nbsp;&nbsp;random_int_list[5]) | 실수, 정수 등 정해진 타입으로 모든 데이터 출력 |
| TUI_SET_ROW_COLS | layout 번호(int), group 번호(int), row 번호(int), row 라벨(str), 데이터(printf 방식) | TUI_SET_ROW_COLS(0, 0, 0, "Joint#1",<br>&nbsp;&nbsp;&nbsp;TUI_COL("0x%04X", statusWord),<br>&nbsp;&nbsp;&nbsp;TUI_COL("%+8.2f", pos_rad[0] * RAD2DEGd),<br>&nbsp;&nbsp;&nbsp;TUI_COL("%+8.2f", pos_rad[1] * RAD2DEGd),<br>&nbsp;&nbsp;&nbsp;TUI_COL("%+8.2f", pos_rad[1] * RAD2DEGd),<br>&nbsp;&nbsp;&nbsp;TUI_COL("%+8.2f", pos_rad[3] * RAD2DEGd),<br>&nbsp;&nbsp;&nbsp;TUI_COL("%+8.2f", pos_rad[4] * RAD2DEGd),<br>&nbsp;&nbsp;&nbsp;TUI_COL("%+8.2f", pos_rad[5] * RAD2DEGd)) | 실수, 정수, Str 등 타입을 혼용해서 출력 |

* <b>(주의) TUI 모드 사용시 아래 예약어들은 키보드 매핑에서 사용할 수 없습니다. (사용은 가능하나 아래 기능과 중복 적용됨!!!)</b>
  * Page Up : (스크롤 수동 모드로 전환 후) 스크롤 영역 페이지 이동 (up)
  * Page Down : (스크롤 수동 모드로 전환 후) 스크롤 영역 페이지 이동 (down)
  * ↑ : (스크롤 수동 모드로 전환 후) 스크롤 영역 라인 이동 (up)
  * ↓ : (스크롤 수동 모드로 전환 후) 스크롤 영역 라인 이동 (down)
  * Home : (스크롤 수동 모드로 전환 후) 스크롤 영역 첫 페이지로 이동
  * End : (스크롤 자동 모드로 전환 후) 스크롤 영역 최신 라인으로 이동
  * [ : 레이아웃 전환 (왼쪽 방향) ex) layout#3 -> layout#2
  * ] : 레이아웃 전환 (오른쪽 방향) ex) layout#1 -> layout#2

### dtDAQ
* 센서 데이터 등의 저장을 지원하기 위한 utility library 입니다.
* 현재 gRPC 기반 네트워크 전송을 지원합니다.
* 메시지 publisher/subscriber 및 RPC server/client 구현을 지원합니다.
* [TODO] Local file로 데이터 저장하는 기능을 구현 계획 중입니다.
* [TODO] HDF5 등 공용 파일 포맷 지원을 계획 중입니다.

## Build & Installation

### Dependancies

#### gRPC
* Network 데이터 전송 및 서비스(RPC) 구현을 위해 gRPC 를 사용합니다.
* gRPC git repository에서 최신 버전 다운로드 받은 후 특정 버전 checkout 하여 빌드 및 설치 합니다.
* gRPC 1.54 버전에서 테스트되었습니다.
* gRPC 빌드 과정에서 sub-modules를 빌드 및 설치합니다.

다음은 터미널에서 grpc 1.54.0 소스 빌드 및 설치과정을 수행합니다. 
빌드된 헤더 파일 및 라이브러리는 $HOME/.local/ART_Framework 폴더 아래 설치됩니다.
```
$ export ARTF_INSTALL_DIR=$HOME/.local/ART_Framework
$ git clone -b v1.54.0 https://github.com/grpc/grpc ./grpc_1.54.0
$ cd grpc_1.54.0
$ git submodule update --init
$ mkdir -p build
$ cd build
$ cmake .. -DgRPC_INSTALL=ON                \
           -DCMAKE_INSTALL_PREFIX=$ARTF_INSTALL_DIR \
           -DCMAKE_BUILD_TYPE=Release       \
           -DgRPC_ABSL_PROVIDER=module     \
           -DgRPC_CARES_PROVIDER=module    \
           -DgRPC_PROTOBUF_PROVIDER=module \
           -DgRPC_RE2_PROVIDER=module      \
           -DgRPC_SSL_PROVIDER=module      \
           -DgRPC_ZLIB_PROVIDER=module
$ make
$ make install
```

#### Protocol buffer
* Protocol buffer SDK 및 컴파일러(protoc)가 설치되어 있어야 합니다. Protocol buffer 설치시 cmake 연동을 위한 module config 파일들도 설치되어야 합니다.
* 개발시 Protocol buffer 3.21.12 버전에서 개발 및 테스트되었습니다.
* gRPC sub-module 빌드/설치시 함께 설치되므로, 따로 설치할 필요 없습니다.

#### spdlog
* dt::Log 모듈이 프로그램 실행 중 발생하는 로그를 파일 혹은 터미널에 출력하기 위해 사용합니다.
* dtCore 설치시 함께 설치됩니다.

#### yaml-cpp
* dt::Utils::Conf 클래스가 yaml 파일 파싱을 위해 사용합니다. yaml에는 프로그램 실행에 필요한 파라미터 등 프로그램 설정이 저장되며, 프로그램은 Conf 클래스를 이용하여 프로그램 시작시 파라미터를 읽어 적용합니다.
* dtCore 설치시 함께 설치됩니다.

#### mcap
* 메시지를 mcap파일에 저장하거나, 저장된 mcap 파일을 읽기 위해 사용하는 optional 라이브러리입니다.
* header-only 라이브러리로 mcap 헤더파일을 $ARTF_INSTALL_DIR/include/mcap/ 폴더 아래 복사합니다.

### dtCore & dtProto
* cmake 빌드 시스템을 이용하여 빌드합니다.
* 소스코드 빌드 후 설치되는 default 디렉토리는, 
  * dtCore 헤더 : $ARTF_INSTALL_DIR/include/dtCore
  * dtProto 헤더 : $ARTF_INSTALL_DIR/include/dtProto
  * dtProto 라이브러리 : $ARTF_INSTALL_DIR/lib
* 설치 디렉토리를 변경하기 위해서는 CMake 옵션 CMAKE_INSTALL_PREFIX 를 변경하세요.
* Build options:

| Option | 내용 | 기본값 |
| :--------- | :--------- | :---------: |
| BUILD_DOCS         | Build documents                                    | OFF |
| BUILD_UNIT_TESTS   | Build unit test or not                             | OFF |
| BUILD_EXAMPLES     | Build examples or not                              | ON |
| BUILD_EXAMPLES_eCAL | Build eCAL examples or not                        | OFF |
| BUILD_EXAMPLES_gRPC | Build gRPC examples or not                        | OFF |
| BUILD_dtProto      | dtProto 헤더 및 라이브러리(libdtproto.a) 빌드           | OFF  |
| BUILD_dtProto_gRPC | dtProto gRPC 헤더 및 라이브러리(libdtproto_grpc.a) 빌드 | OFF |
| GIT_SUBMODULE     | Get and build git submodules(spdlog and yaml-cpp)           | ON |


* cmake command line interface에서는 다음과 같이 실행합니다.

```
> cd /to/dtCore/source/repository
> cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTAL_PREFIX=$ARTF_INSTALL_DIR
> cmake --build build -j8
> cmake --build build --target doc
> sudo cmake --build build --target install
```

* Doxygen 기반 문서화를 지원합니다.
* 문서를 생성하기 위해서 다음 명령을 실행하세요.
```
> cd /to/dtCore/source/repository
> cmake -S . -B build -DBUILD_DOCS=ON
> cmake --build build --target doc
```
* 문서는 build/doc/ 아래 html형식(index.html)으로 생성됩니다.


## Contacts
For more information go to [ART Framework dtCore git repository](https://gitlabee.hmg-corp.io/rlab/art/ctrlpart/project/arch/03-art-framework/dtcore).
