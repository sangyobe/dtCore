#### [v1.2.1]
(2024/6/5)
##### dtServiceListenerGrpc
- Stop() 호출시 연결된 session에 대한 call을 즉시 cancel함.
- FINISH 상태의 session에 대해 compeletion queue event 처리하지 않음(중복된 Finish() 호출 오류 fix)

#### [v1.2.0]
(2024/5/13)
- dtcore, dtcore_grpc 라이브러리(cmake target) 추가
- dtThread, dtUtils 헤더 및 소스코드 추가