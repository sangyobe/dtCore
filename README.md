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

#### Protocol buffer
* Protocol buffer SDK 및 컴파일러(protoc)가 설치되어 있어야 합니다. Protocol buffer 설치시 cmake 연동을 위한 module config 파일들도 설치되어야 합니다.
* 개발시 Protocol buffer 3.21.12 버전에서 개발 및 테스트되었습니다.
* gRPC sub-module 빌드/설치시 함께 설치되므로, 따로 설치할 필요 없습니다.

#### spdlog
* dt::Log 모듈이 프로그램 실행 중 발생하는 로그를 파일 혹은 터미널에 출력하기 위해 사용합니다.

#### yaml-cpp
* dt::Utils::Conf 클래스가 yaml 파일 파싱을 위해 사용합니다. yaml에는 프로그램 실행에 필요한 파라미터 등 프로그램 설정이 저장되며, 프로그램은 Conf 클래스를 이용하여 프로그램 시작시 파라미터를 읽어 적용합니다.

#### mcap
* 메시지를 mcap파일에 저장하거나, 저장된 mcap 파일을 읽기 위해 사용합니다.

### dtCore & dtProto
* cmake 빌드 시스템을 이용하여 빌드합니다.
* 소스코드 빌드 후 설치되는 default 디렉토리는, 
  * dtCore 헤더 : /usr/local/include/dtCore
  * dtProto 헤더 : /usr/local/include/dtProto
  * dtProto 라이브러리 : /usr/local/lib
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
> cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTAL_PREFIX=/usr/local
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
