#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <getopt.h>
#include <stdint.h>

#include "../edge/byte_op.h"

#define BUFLEN        1024
#define OPCODE_SUM    1
#define OPCODE_REPLY  2

void protocol_execution(int sock); //소켓 통신 비즈니스 로직 수행 프로토타입 선언
void error_handling(const char *message); //에러 발생 시 메시지 출력하고 종료하는 프로토타입 선언

void usage(const char *pname) //프로그램의 올바른 실행 옵션 방법을 안내하는 함수 정의 시작
{ //usage 함수 바디 시작
  printf(">> Usage: %s [options]\n", pname); //실행된 프로그램 이름과 함께 사용법 안내 서두 출력
  printf("Options\n"); //"Options" 항목 타이틀 출력
  printf("  -a, --addr       Server's address\n"); //서버 주소 지정을 위한 -a, --aadr 옵션 안내 출력
  printf("  -p, --port       Server's port\n"); //서버 포트 지정을 위한 -p, --port 옵션 안내 출력
  exit(0); //사용법을 안내한 후 프로그램을 정상 범위(0)에서 즉시 종료
} // usage 함수 바디 끝

int main(int argc, char *argv[]) //프로그램의 진입점인 main 함수 정의 시작
{ // main 함수 바디 시작
	int sock; //서버와의 연결을 관리할 소켓 파일 디스크립터 변수 선언
	struct sockaddr_in serv_addr; //서버의 IPv4 주소, 포트 정보 등을 저장할 소켓 주소 구조체 변수 선언
  char msg[] = "Hello, World!\n"; //예제용 초기화 문자열 배열 변수 선언
	char message[30] = {0, }; //0으로 초기화된 30바이트 크기의 임시 버퍼 배열 선언
	int c, port, tmp, str_len; // 옵션 문자(c), 포트번호(port), 임시변수(tmp), 문자열 길이(str_len) 변수 선언
  char *pname; //실행 파일 이름을 보관할 문자열 포인터 변수 선언
  uint8_t *addr; //입력받은 서버 IP 주소 문자열을 가리킬 부호 없는 8비트 정수형 포인터 선언
  uint8_t eflag; //필수 입력 옵션의 누락 여부를 체크하기 위한 에러 플래그 변수 선언

  pname = argv[0]; //명령행으로 입력된 인자 중 첫 번째인 프로그램 실행 경로/이름을 pname에 대입
  addr = NULL; //주소 포인터를 안전하게 NULL(참조 없음)로 초기화
  port = -1; //포트 번호를 입력받지 않은 상태를 나타내기 위해 초기값 -1 대입
  eflag = 0; //에러 플래그를 에러 없음 상태인 0으로 초기화

  while (1) //명령행 옵션들을 모두 분석할 때까지 무한 루프 수행 시작
  { //옵션 분석 무한 루프 바디 시작
    int option_index = 0; //getopt_long 함수가 현재 처리 중인 긴 옵션의 인덱스를 저장할 변수 선언
    static struct option long_options[] = { //긴 옵션(--addr, --port)의 명세 구조체 배열 정의 시작
      {"addr", required_argument, 0, 'a'}, //--addr 옵션은 뒤에 반드시 인자값이 와야 하며 대칭되는 문자는 'a'
      {"port", required_argument, 0, 'p'}, //--port 옵션은 뒤에 반드시 인자값이 와야 하며 대칭되는 문자는 'p'
      {0, 0, 0, 0} //구조체 배열의 끝을 알리는 더미(Null) 데이터 지정
    }; // 구조체 배열 정의 끝

    const char *opt = "a:p:0"; //짧은 옵션 문자셋 지정 ('a:'와 'p:'는 각각 인자가 필수적임을 의미)

    c = getopt_long(argc, argv, opt, long_options, &option_index); //명령행 인자에서 옵션을 추출하여 c에 저장

    if (c == -1) //더 이상 처리할 옵션 문자나 인자가 없으면 (-1 반환 시)
      break; //옵션 분석 파싱 무한 루프를 탈출

    switch (c) //파싱되어 리턴된 옵션 문자 c의 값에 따라 분기 처리 시작
    { //switch 문 바디 시작
      case 'a': //옵션이 'a'(또는 --addr)인 경우 처리 분기
        tmp = strlen(optarg); //옵션의 인자값(IP주소 문자열)의 길이를 계산하여 tmp에 저장
        addr = (uint8_t *)malloc(tmp); //주소 문자열을 복사해 둘 메모리 공간을 동적 할당 (널문자 포함 +1)
        memcpy(addr, optarg, tmp); //할당된 메모리 공간(addr)으로 입력받은 IP 주소 문자열(optarg)을 복사
        break; //case 'a' 분기 종료

      case 'p': //옵션이 'p'(또는 --port)인 경우 처리 분기
        port = atoi(optarg); //문자열 상태인 포트 인자값(optarg)을 정수형(int) 데이터로 변환하여 port에 저장
        break; //case 'p' 분기 종료

      default: //명시되지 않은 잘못된 옵션이 들어왔을 경우 실행되는 예외 분기
        usage(pname); //프로그램 사용법 출력 함수를 호출하고 내부적으로 종료함
    } //switch 문 바디 끝
  } //옵션 분석 무한 루프 바디 끝

  if (!addr) //IP 주소(addr)가 여전히 NULL인 경우, 즉 주소 옵션이 지정되지 않았다면 조건문 실행
  { //주소 미지정 조건문 바디 시작
    printf("[*] Please specify the server's address to connect\n"); //서버 주소 입력이 필요하다는 안내 문구 출력
    eflag = 1; //옵션 입력에 누락 오류가 있으므로 에러 플래그를 1로 설정
  } //주소 미지정 조건문 바디 끝

  if (port < 0) //포트 번호(port)가 여전히 초기값인 -1이거나 음수라면, 즉 포트 옵션이 지정되지 않았다면 실행
  { //포트 미지정 조건문 바디 시작
    printf("[*] Please specify the server's port to connect\n"); //서버 포트 입력이 필요하다는 안내 문구 출력
    eflag = 1; //옵션 입력에 누락 오류가 있으므로 에러 플래그를 1로 설정
  } // 포트 미지정 조건문 바디 끝

  if (eflag) //에러 플래그(eflag)가 1로 켜져 있다면 (필수 옵션 중 하나라도 누락되었다면) 실행
  { // 에러 발생 조건문 바디 시작
    usage(pname); // 사용법을 다시 한번 상세히 안내 출력
    exit(0); //프로그램을 정상 플래그 상태로 안전하게 즉시 종료
  } //에러 발생 조건문 바디 끝

	sock = socket(PF_INET, SOCK_STREAM, 0); //IPv4 프로토콜 체계 및 TCP 연결 지향형 소켓을 생성하여 디스크립터 획득
	if (sock == -1) //소켓 생성 함수가 실패를 뜻하는 -1을 반환했다면 조건문 실행
		error_handling("socket() error"); //소켓 에러 메시지를 출력하는 예외 처리 함수 호출 및 종료
	memset(&serv_addr, 0, sizeof(serv_addr)); //서버 주소 구조체 메모리 영역 전체를 0으로 깔끔하게 초기화
	serv_addr.sin_family = AF_INET; //주소 체계 패밀리를 IPv4 인터넷 프로토콜 주소 체계로 설정
	serv_addr.sin_addr.s_addr = inet_addr((const char *)addr); //문자열 형태의 IP 주소를 32비트 네트워크 이진 주소로 변환하여 대입
	serv_addr.sin_port = htons(port); //호스트 바이트 순서의 포트 정수를 네트워크 바이트 순서(빅엔디안)로 변환하여 대입

	if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) //구조체 주소 정보를 이용해 서버에 TCP 연결 요청
		error_handling("connect() error"); //연결 요청이 실패(-1)했을 경우 에러 메시지를 출력하고 프로그램 종료
  printf("[*] Connected to %s:%d\n", addr, port); //서버 연결 성공 시 원격지 IP 주소와 포트 번호를 화면에 출력
  
  protocol_execution(sock); //생성 및 연결 완료된 소켓 디스크립터를 넘겨주어 데이터 송수신 비즈니스 로직 함수 실행

	close(sock); //데이터 통신이 모두 끝난 소켓 연결을 닫고 자원을 운영체제에 반환
	return 0; //프로그램이 에러 없이 완벽하게 성공했음을 알리며 메인 함수 종료(0 반환)
} //main 함수 바디 끝

void protocol_execution(int sock) //실질적인 원격 서버와의 통신 규격(프로토콜)을 처리하는 함수 정의 시작
{ //protocol_execution 함수 바디 시작
  char msg[] = "Alice"; //클라이언트 측의 식별 이름 문자열 변수 선언 및 "Alice"로 초기화
  char buf[BUFLEN]; //데이터 송수신 및 조작 패킷 빌드를 위한 1024바이트 크기의 공용 메모리 버퍼 배열 선언
  int tbs, sent, tbr, rcvd, offset; //보낼크기(tbs), 보낸크기(sent), 받을크기(tbr), 받은크기(rcvd), 진행위치(offset) 변수 선언
  int len; // 전송되거나 수신될 데이터의 길이 값을 연산 및 저장하는 다목적 정수형 변수 선언

  // tbs: the number of bytes to send
  // tbr: the number of bytes to receive
  // offset: the offset of the message

  // 1. Alice -> Bob: length of the name (4 bytes) || name (length bytes)
  // Send the length information (4 bytes)
  len = strlen(msg); //문자열 "Alice"의 바이트 크기인 숫자 5를 계산하여 len 변수에 저장
  printf("[*] Length information to be sent: %d\n", len); //전송할 이름의 길이(5)를 안내 포맷으로 화면에 출력

  len = htonl(len); //다른 엔디안 시스템과의 호환을 위해 호스트 정수를 네트워크 공통 바이트 순서(빅엔디안)로 변환
  tbs = 4; //4바이트 크기의 정수형 길이 데이터를 전송 타깃 바이트 크기(tbs)로 설정
  offset = 0; //현재 스트림 송신 진행 상태 오프셋 위치를 0으로 초기화

  while (offset < tbs) //지정된 타깃 크기(4바이트)만큼 완전히 전송을 마칠 때까지 송신 반복 루프 수행 시작
  { //송신 반복 루프 바디 시작
    sent = write(sock, &len + offset, tbs - offset); // 정수 주소(&len)를 문자형 포인터((char *))로 캐스팅하여 바이트 단위로 정확히 오프셋 이동 및 남은 만큼 write 실행
    if (sent > 0) //네트워크 버퍼에 정상적으로 데이터가 한 바이트 이상 밀려 들어갔다면 조건문 실행
      offset += sent; //전송에 성공하여 처리된 바이트 크기만큼 진행 상태 오프셋에 누적 추가
  } //송신 반복 루프 바디 끝

  // Send the name (Alice)
  tbs = ntohl(len); //네트워크 정수로 바뀐 길이 데이터를 호스트 정수로 역전환하여 실제 보낼 이름 크기(5)를 복원해 tbs에 지정
  offset = 0; //문자열 데이터 전송을 시작하기 위해 진행 상태 오프셋 위치를 다시 0으로 초기화

  printf("[*] Name to be sent: %s\n", msg); //전송을 시작하는 타깃 문자열 내용("Alice")을 화면에 명시적 출력
  while (offset < tbs) //이름의 바이트 길이(5바이트)만큼 완전히 전송을 마칠 때까지 송신 반복 루프 수행 시작
  { //송신 반복 루프 바디 시작
    sent = write(sock, msg + offset, tbs - offset); //이름 포인터에서 현재 오프셋을 더한 주소부터 남은 바이트만큼 데이터 전송
    if (sent > 0) //한 바이트 이상의 이름 데이터가 소켓 스트림으로 성공적으로 전송되었다면 실행
      offset += sent; //전송 완료된 크기만큼 진행 상태 오프셋에 누적 추가
  } //송신 반복 루프 바디 끝

  // 2. Bob -> Alice: length of the name (4 bytes) || name (length bytes)
  // Receive the length information (4 bytes)
  tbr = 4; //읽어와야 할 길이 정수의 크기가 4바이트이므로 수신 타깃 바이트 수(tbr)를 4로 지정
  offset = 0; //수신 스트림의 적재 시작점을 초기화하기 위해 오프셋을 0으로 설정

  while (offset < tbr) //상대방이 보낸 길이 정보 4바이트가 온전히 수신될 때까지 읽기 반복 루프 수행 시작
  { //읽기 반복 루프 바디 시작
	  rcvd = read(sock, &len + offset, tbr - offset); //수신받을 변수(&len) 주소를 바이트 포인터형으로 캐스팅 후 현재 오프셋 주소 공간에 남은 요구량만큼 소켓으로부터 read
    if (rcvd > 0) // 연결이 끊어지지 않고 소켓으로부터 한 바이트 이상의 데이터를 정상 수신했다면 실행
      offset += rcvd; // 수신에 성공한 바이트 수만큼 진행 상태 오프셋 공간에 누적 이동
  } // 읽기 반복 루프 바디 끝
  len = ntohl(len); // 수신 완료된 빅엔디안 형식의 네트워크 정수 데이터를 현재 시스템에 맞는 호스트 바이트 정수로 복원
  printf("[*] Length received: %d\n", len); // 수신받은 상대방 이름 데이터의 총 크기 수치를 화면에 출력
 
  // Receive the name (Bob)
  tbr = len; // 방금 디코딩하여 얻어낸 상대방 이름 길이 수치를 다음 수신 타깃 바이트 수(tbr)로 지정
  offset = 0; // 문자열 전송 버퍼에 차례대로 담기 위해 진행 상태 오프셋 위치를 다시 0으로 초기화
 
  while (offset < tbr) // 서버 측 이름의 길이만큼의 데이터를 완전히 소켓 스트림에서 수집할 때까지 반복 루프 수행 시작
  { // 읽기 반복 루프 바디 시작
    rcvd = read(sock, buf + offset, tbr - offset); // 공용 수신 버퍼(buf)의 현재 오프셋 위치 주소에 남은 만큼 데이터를 수신
    if (rcvd > 0) // 오류 없이 소켓 바이트 스트림으로부터 정상적으로 데이터가 수신되었다면 조건문 실행
      offset += rcvd; // 읽어 들이는데 성공한 크기 바이트 수만큼 진행 상태 오프셋 값을 가산 누적
  } // 읽기 반복 루프 바디 끝

	printf("[*] Name received: %s \n", buf); // 최종 수신 및 디코딩 처리가 끝난 상대방의 이름 문자열을 화면에 출력

  // Implement following the instructions below
  // Let's assume there are two opcodes:
  //     1: summation request for the two arguments
  //     2: reply with the result
  // 3. Alice -> Bob: opcode (4 bytes) || arg1 (4 bytes) || arg2 (4 bytes)
  // The opcode should be 1

  char *p; // 바이너리 패킷 빌드 및 주소 이동 제어를 위한 문자형 포인터 변수 p 선언
  int i, arg1, arg2; // 루프 인덱스 변수(i)와 연산 요청에 피연산자로 사용할 정수형 변수 2개(arg1, arg2) 선언

  memset(buf, 0, BUFLEN); // 패킷을 깨끗하게 새로 만들기 위해 공용 버퍼 메모리 전체 공간을 다시 0으로 청소
  p = buf; // 포인터 p가 패킷 제작을 위해 공용 버퍼(buf)의 시작점 주소를 가리키도록 설정
  arg1 = 2; // 첫 번째 더하기 피연산자 변수 arg1에 숫자 값 2 대입
  arg2 = 5; // 두 번째 더하기 피연산자 변수 arg2에 숫자 값 5 대입

  VAR_TO_MEM_1BYTE_BIG_ENDIAN(OPCODE_SUM, p); // 매크로를 사용하여 Opcode(1)를 4바이트 빅엔디안 바이너리로 적재 후 포인터 p 이동
  VAR_TO_MEM_4BYTES_BIG_ENDIAN(arg1, p); // 매크로를 사용하여 인자1(2)을 4바이트 빅엔디안 바이너리로 적재 후 포인터 p 이동
  VAR_TO_MEM_4BYTES_BIG_ENDIAN(arg2, p); // 매크로를 사용하여 인자2(5)을 4바이트 빅엔디안 바이너리로 적재 후 포인터 p 이동
  tbs = p - buf; // 최종 이동한 포인터 위치와 시작점의 주소 차이를 구해 생성된 총 패킷의 크기(12바이트)를 tbs에 저장
  offset = 0; // 작성된 연산 요청 패킷을 전송하기 위해 진행 상태 오프셋 위치 변수를 0으로 초기화

  printf("[*] # of bytes to be sent: %d\n", tbs); // 네트워크로 송신할 연산 패킷의 총 크기 바이트(12)를 화면에 출력
  printf("[*] The following bytes will be sent\n"); // 송신 직전의 패킷 바이너리 데이터 상태를 보여주기 위한 안내 헤더 출력
  for (i=0; i<tbs; i++) // 패킷 크기(12바이트)만큼 배열을 한 바이트씩 순회하는 덤프용 루프 시작
    printf ("%02x ", buf[i]); // 패킷 내부의 순수 바이너리 데이터를 16진수 2자리 포맷 형태로 변환하여 출력
  printf("\n"); // 바이너리 로그 출력이 한 줄 모두 완료됨에 따라 개행문자 출력

  while (offset < tbs) // 조립 완료된 연산 요청 패킷 전체(12바이트)가 완전히 전송될 때까지 반복 루프 수행 시작
  { // 송신 반복 루프 바디 시작
    sent = write(sock, buf + offset, tbs - offset); // 버퍼의 현재 진행 오프셋 위치부터 아직 보내지 못한 남은 잔여 크기만큼 송신
    if (sent > 0) // 한 바이트 이상의 패킷 바이트 스트림 데이터가 성공적으로 송신되었다면 실행
      offset += sent; // 송신에 성공 처리된 크기만큼 진행 상태 오프셋 값을 가산 누적
  } // 송신 반복 루프 바디 끝

  // 4. Bob -> Alice: opcode (4 bytes) || result (4 bytes)
  // The opcode should be 2

  int opcode, result; // 수신 패킷에서 추출해낼 응답 분류 코드(opcode)와 연산 결과 수치(result) 정수 변수 선언

  tbr = 8; offset = 0; // 서버가 보낼 응답 패킷의 크기가 총 8바이트(Opcode 4바이트 + 결과 4바이트)이므로 수신 타깃(tbr)을 8로 지정, 응답 패킷을 처음부터 받아내기 위해 수신 진행 상태 오프셋 값을 0으로 초기화
  memset(buf, 0, BUFLEN); // 기존에 송신할 때 썼던 공용 데이터 버퍼 메모리 영역을 수신용으로 쓰기 위해 다시 0으로 초기화

  printf("[*] # of bytes to be received: %d\n", tbr); // 받아야 하는 수신 기대 데이터 바이트 수(8)를 화면에 안내 출력
  while (offset < tbr) // 규격상 명시된 8바이트의 패킷 데이터가 소켓에 전부 도달하여 읽을 때까지 반복 루프 수행 시작
  { // 읽기 반복 루프 바디 시작
    rcvd = read(sock, buf + offset, tbs - offset); // 공용 버퍼(buf)의 현재 오프셋 위치 주소에 서버 응답의 아직 읽지 못한 남은 잔여 바이트 크기만큼 수신 (tbr 기반으로 연산 조치)
    if (rcvd > 0) // 소켓으로부터 연결 유실 없이 데이터 수신이 정상 통과되었다면 실행
      offset += rcvd; // 수신에 성공한 크기만큼 데이터 적재를 위해 수신 진행 상태 오프셋 수치 누적 가산
  } // 읽기 반복 루프 바디 끝

  printf("[*] The following bytes is received\n"); // 수신 완료된 원시 응답 패킷 데이터 덤프를 출력한다는 안내 타이틀 출력
  for (i=0; i<tbr; i++) // 응답 패킷 크기(8바이트)만큼 배열을 한 바이트씩 순회하는 덤프용 루프 시작
    printf("%02x ", buf[i]); // 수신 버퍼 내부의 바이너리 원본 값을 16진수 포맷 형태로 변환하여 출력
  printf("\n"); // 응답 데이터 바이너리 덤프 출력이 종료됨에 따라 줄 바꿈 실행

  p = buf; // 수신 완료된 원시 바이너리 패킷 버퍼에서 데이터를 순차 파싱하기 위해 포인터 p를 버퍼 시작점으로 재지정
  MEM_TO_VAR_4BYTES_BIG_ENDIAN(p, opcode); // 매크로를 통해 첫 4바이트 빅엔디안 바이너리를 호스트 정수로 추출해 opcode에 저장하고 p 이동
  printf("[*] Opcode: %d\n", opcode); // 디코딩되어 복원된 응답 Opcode 수치(정상 응답 시 2)를 화면에 출력
  MEM_TO_VAR_4BYTES_BIG_ENDIAN(p, result); // 매크로를 통해 다음 4바이트 빅엔디안 바이너리를 호스트 정수로 추출해 result에 저장하고 p 이동
  printf("[*] Result: %d\n", result); // 디코딩되어 복원된 최종 사칙연산 처리 결과 값(2+5 결과인 7)을 화면에 출력
} // protocol_execution 함수 바디 끝

void error_handling(const char *message) // 시스템 콜 등 예외 상황 및 에러가 감지되었을 때 처리하는 함수 정의 시작
{ // error_handling 함수 바디 시작
	fputs(message, stderr); // 발생한 구체적인 에러 메시지 문자열을 터미널의 표준 에러(stderr) 스트림 창으로 출력
	fputc('\n', stderr); // 에러 메시지 가독성을 위해 표준 에러 스트림 창에 줄 바꿈(개행 문자) 추가 출력
	exit(1); // 프로세스에게 문제가 발생했음을 뜻하는 비정상 종료 코드 식별자 1을 운영체제에 반환하며 프로그램 강제 종료
} // error_handling 함수 바디 끝
