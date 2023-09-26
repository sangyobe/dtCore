# dtCore
**dtCore is a C++ (template) library for ART software implementations such as definitions of basic data structures, messaging framework, trajectory interpolator, file parsers, data logger, etc.**

## Build & Installation
* cmake 빌드 시스템을 이용하여 빌드합니다.
* 소스코드 빌드 후 설치되는 default 디렉토리는, 
  * dtCore 헤더 : /usr/local/include/dtCore
  * dtProto 헤더 : /usr/local/include/dtProto
  * dtProto 라이브러리 : /usr/local/lib
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
| dtCore_USE_EIGEN3  | Use Eigen3 as the default math library             | OFF |

* cmake command line interface에서는 다음과 같이 적용합니다.

```
> cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_EXAMPLES=ON
```

### dtTrajectory
* dtPolynomialTrajectory, dtBezierTrajectory, dtScurveTrajectory 및 dtTrajectoryList 는 별도의 빌드 과정이 필요없이 사용할 수 있습니다. 단, 시스템 폴더에 설치(헤더 파일 복사)하기 위해 다음 스크립트를 실행하세요.

```
> cd /to/dtCore/Directory
> mkdir build
> cd build
> cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_EXAMPLES=ON
> make -j4
> sudo make install
```

* 위 스크립트 실행시 dtTrajectory 헤더파일이 /usr/local/include/dtCore (Linux 기준) 아래 설치됩니다.

* [TODO] 개발 버전에서는 dtHtransformTrajectory, dtOrientationTrajectory, dtVectorPolynomialTrajectory 테스트를 위해 dtMath가 Eigen3를 사용하고 있습니다. 따라서 빌드 및 사용자 코드에서 헤더를 포함하여 개발하기 위해서는 Eigen3가 시스템에 설치되어 있어야 하며, 시스템 환경 변수에 $EIGEN_ROOT를 Eigen3 설치 디렉토리로 설정합니다. 예를 들어 Eigen3 Dense 헤더파일이 /usr/local/eigen-3.4.0/Eigen/Dense 과 같이 설치된 경우 $EIGEN_ROOT 를 /usr/local/eigen-3.4.0 로 설정합니다. 

### dtProto
Protocol Buffer 기반으로 C++ 메시지 정의 및 serialization을 지원합니다. 

* Protocol buffer SDK 및 컴파일러(protoc)가 설치되어 있어야 합니다. Protocol buffer 설치시 cmake 연동을 위한 module config 파일들도 설치되어야 합니다.
* 개발시 protoc 3.21.12 버전에서 개발 및 테스트되었습니다.
* 유저코드에서 기능을 사용하기 위해서는 헤더파일(/include/dtProto/*.h)과 빌드된 라이브러리(/lib/libart_protocol.a)가 필요합니다.
* [TODO] Transport layer로 gRPC, eCAL을 고려 중이며 현재 테스트 중입니다.
* [TODO] eCAL 의 경우 protobuf 동적링크(shared) 라이브러리를 사용합니다. 이에 관한 빌드 옵션 및 라이브러리 버전간 충돌 문제가 발생할 수 있어 테스트 중입니다.

* dtProto 메시지 정의 파일(헤더 파일) 및 라이브러리(libdtProto.a) 빌드 및 설치를 위해 다음 스크립트를 실행하세요.

```
> cd /to/dtCore/Directory
> mkdir build
> cd build
> cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_dtProto=ON
> make -j4
> sudo make install
```

### dtMath
dtTrajectory 인터페이스 Test를 위한 임시 헤더파일입니다. 이외 목적으로 프로젝트에 포함하지 마세요.

## Dependancies
#### Protocol buffer
* Version: 3.21

#### gRPC
* Version: 1.54

#### Eigen
* Version: 3.4
* 
## Contacts
For more information go to [ART Framework dtCore git repository](https://cody-escm.autoever.com/rlab/art/ctrlpart/personal/sangyup-yi/dtcore) .

For ***pull request***, ***bug reports***, and ***feature requests***, go to https://cody-escm.autoever.com/rlab/art/ctrlpart/personal/sangyup-yi/dtcore.
