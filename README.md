# dtCore
**dtCore is a C++ (template) library for ART software implementations such as definitions of basic data structures, messaging framework, trajectory interpolator, file parsers, data logger, etc.**

## Build & Installation
아직 설치에 대한 정책이 정해지지 않았습니다. dtTrajectory 등 일부 기능은 빌드(build) 없이 사용이 가능합니다. 
### dtTrajectory

* 현재 공식 릴리즈 전입니다. 개발 버전에서는 테스트를 위해 dtMath가 Eigen3를 사용하고 있습니다. 따라서 빌드 및 사용자 코드에서 헤더를 포함하여 개발하기 위해서는 Eigen3가 시스템에 설치되어 있어야 하며, 시스템 환경 변수에 $EIGEN_ROOT를 Eigen3 설치 디렉토리로 설정합니다. 예를 들어 Eigen3 Dense 헤더파일이 /usr/local/eigen-3.4.0/Eigen/Dense 과 같이 설치된 경우 $EIGEN_ROOT 를 /usr/local/eigen-3.4.0 로 설정합니다. 

### dtProto
Protocol Buffer 기반으로 C++ 메시지 정의 및 serialization을 지원합니다. 

* Protocol buffer SDK 및 컴파일러(protoc)가 설치되어 있어야 합니다. Protocol buffer 설치시 cmake 연동을 위한 module config 파일들도 설치되어야 합니다.
* 개발시 protoc 3.21.12 버전에서 개발 및 테스트되었습니다.
* 유저코드에서 기능을 사용하기 위해서는 헤더파일(/include/dtProto/*.h)과 빌드된 라이브러리(/lib/libart_protocol.a)가 필요합니다.
* [TODO] Transport layer로 gRPC, eCAL을 고려 중이며 현재 테스트 중입니다.

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
