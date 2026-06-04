#include "network_manager.h"
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <assert.h>

#include "opcode.h"
using namespace std;

NetworkManager::NetworkManager() 
{
  this->sock = -1;
  this->addr = NULL;
  this->port = -1;
}

NetworkManager::NetworkManager(const char *addr, int port)
{
  this->sock = -1;
  this->addr = addr;
  this->port = port;
}

void NetworkManager::setAddress(const char *addr)
{
  this->addr = addr;
}

const char *NetworkManager::getAddress()
{
  return this->addr;
}

void NetworkManager::setPort(int port)
{
  this->port = port;
}

int NetworkManager::getPort()
{
  return this->port;
}

int NetworkManager::init()
{
	struct sockaddr_in serv_addr;

	this->sock = socket(PF_INET, SOCK_STREAM, 0);
	if (this->sock == FAILURE)
  {
    cout << "[*] Error: socket() error" << endl;
    cout << "[*] Please try again" << endl;
    exit(1);
  }

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(this->addr);
	serv_addr.sin_port = htons(this->port);

	if (connect(this->sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == FAILURE)
  {
    cout << "[*] Error: connect() error" << endl;
    cout << "[*] Please try again" << endl;
    exit(1);
  }
	
  cout << "[*] Connected to " << this->addr << ":" << this->port << endl;

  return sock;
}

// TODO: You should revise the following code
int NetworkManager::sendData(uint8_t *data, int dlen)  // ProcessManager에서 처리된 데이터를 서버로 보내는 함수
{
  int sock, tbs, sent, offset, num, jlen;  // 사용할 소켓 번호, 보낼 바이트 수, 실제 전송된 바이트 수, 전송 위치 등을 저장할 변수들
  unsigned char opcode;  // 서버로 보낼 opcode 값을 저장할 변수
  uint8_t n[4];  // 4바이트 크기의 임시 배열
  uint8_t *p;  // uint8_t 포인터 변수

  sock = this->sock;  // 객체에 저장된 소켓 번호를 지역 변수 sock에 복사
  // Example) data (processed by ProcessManager) consists of:
  // Example) minimum temperature (1 byte) || minimum humidity (1 byte) || minimum power (2 bytes) || month (1 byte)
  // Example) edge -> server: opcode (OPCODE_DATA, 1 byte)
  opcode = OPCODE_DATA;  // 전송할 opcode를 데이터 전송을 의미하는 OPCODE_DATA로 설정
  tbs = 1; offset = 0;  // 보낼 바이트 수를 1바이트로 설정하고, 전송 시작 위치 offset을 0으로 초기화
  while (offset < tbs)  // offset이 tbs보다 작으면 아직 전송할 데이터가 남아 있다는 뜻이므로 반복
  {
    sent = write(sock, &opcode + offset, tbs - offset);  // opcode의 offset 위치부터 남은 바이트 수만큼 소켓으로 전송
    if (sent > 0)  // 실제로 전송된 바이트 수가 0보다 크면
      offset += sent;  // 전송된 만큼 offset을 증가시켜 다음 전송 위치를 갱신
  }
  assert(offset == tbs);  // 실제 전송한 총 바이트 수가 보내야 할 바이트 수와 같은지 확인

  // Example) edge -> server: temperature (1 byte) || humidity (1 byte) || power (2 bytes) || month (1 byte)
  tbs = 5; offset = 0;  // 실제 데이터 크기를 5바이트로 설정하고, 전송 시작 위치 offset을 0으로 초기화
  while (offset < tbs)  // offset이 tbs보다 작으면 아직 전송할 데이터가 남아 있다는 뜻이므로 반복
  {
    sent = write(sock, data + offset, tbs - offset);  // data의 offset 위치부터 남은 바이트 수만큼 소켓으로 전송
    if (sent > 0)  // 실제로 전송된 바이트 수가 0보다 크면
      offset += sent;  // 전송된 만큼 offset을 증가시켜 다음 전송 위치를 갱신
  }
  assert(offset == tbs);  // 실제 전송한 총 바이트 수가 보내야 할 바이트 수와 같은지 확인

  return 0;  // 함수가 정상적으로 끝났음을 의미하는 0을 반환
}

// TODO: Please revise or implement this function as you want. You can also remove this function if it is not needed
uint8_t NetworkManager::receiveCommand()   // 서버로부터 명령 opcode를 받을 때까지 기다리고, 받은 opcode를 반환하는 함수
{
  int sock;  // 현재 통신에 사용할 소켓 번호를 저장할 변수
  uint8_t opcode;  // 서버로부터 받을 명령 코드를 저장할 변수
  uint8_t *p;  // uint8_t 포인터 변수

  sock = this->sock;  // 객체에 저장되어 있는 소켓 번호를 지역 변수 sock에 복
  opcode = OPCODE_WAIT;  // 처음 상태를 대기 상태인 OPCODE_WAIT으로 설정

  while (opcode == OPCODE_WAIT)  // opcode가 OPCODE_WAIT인 동안 서버로부터 새로운 opcode를 계속 읽음
    read(sock, &opcode, 1);  // 소켓에서 1바이트를 읽어 opcode 변수에 저장

  assert(opcode == OPCODE_DONE || opcode == OPCODE_QUIT) ;  // 받은 opcode가 OPCODE_DONE 또는 OPCODE_QUIT인지 확인

  return opcode;  // 최종적으로 받은 opcode 값을 반환
}
