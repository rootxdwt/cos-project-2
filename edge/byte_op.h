#ifndef __BYTE_OP_H__  // __BYTE_OP_H__가 아직 정의되지 않았다면 아래 코드를 포함
#define __BYTE_OP_H__  // __BYTE_OP_H__를 정의하여 헤더 파일이 중복 포함되는 것을 방지

#include <cstdio>  // printf 같은 C 스타일 입출력 함수를 사용하기 위한 헤더 파일
#include <cstring>  // 문자열 및 메모리 관련 함수를 사용하기 위한 헤더 파일

#define VAR_TO_MEM_1BYTE_BIG_ENDIAN(v, p) \  // 변수 v를 1바이트 big-endian 형식으로 메모리 p에 저장하는 매크로
  *(p++) = v & 0xff;  // v의 하위 1바이트만 추출하여 p가 가리키는 위치에 저장하고, p를 다음 위치로 이동

#define VAR_TO_MEM_2BYTES_BIG_ENDIAN(v, p) \  // 변수 v를 2바이트 big-endian 형식으로 메모리 p에 저장하는 매크로
  *(p++) = (v >> 8) & 0xff; *(p++) = v & 0xff;   // 상위 바이트를 먼저 저장하고, 그 다음 하위 바이트를 저장

#define VAR_TO_MEM_4BYTES_BIG_ENDIAN(v, p) \  // 변수 v를 4바이트 big-endian 형식으로 메모리 p에 저장하는 매크로
  *(p++) = (v >> 24) & 0xff; *(p++) = (v >> 16) & 0xff; *(p++) = (v >> 8) & 0xff; *(p++) = v & 0xff;  // 가장 상위 바이트부터 가장 하위 바이트까지 순서대로 메모리에 저장

#define MEM_TO_VAR_1BYTE_BIG_ENDIAN(p, v) \  // 메모리 p에서 1바이트를 읽어 변수 v에 저장하는 매크로
  v = (p[0] & 0xff); p += 1;  // p가 가리키는 1바이트 값을 v에 저장하고, p를 1바이트 뒤로 이동

#define MEM_TO_VAR_2BYTES_BIG_ENDIAN(p, v) \  // 메모리 p에서 2바이트 big-endian 데이터를 읽어 변수 v에 저장하는 매크로
  v = ((p[0] & 0xff) << 8) | (p[1] & 0xff); p += 2;  // 첫 번째 바이트를 상위 바이트로, 두 번째 바이트를 하위 바이트로 합쳐 v에 저장

#define MEM_TO_VAR_4BYTES_BIG_ENDIAN(p, v) \  // 메모리 p에서 4바이트 big-endian 데이터를 읽어 변수 v에 저장하는 매크로
  v = ((p[0] & 0xff) << 24) | ((p[1] & 0xff) << 16) | ((p[2] & 0xff) << 8) | (p[3] & 0xff); p += 4;  // 4개의 바이트를 big-endian 순서로 합쳐 하나의 4바이트 값으로 만듦

#define PRINT_MEM(p, len) \  // 메모리 p의 내용을 len 길이만큼 16진수 형태로 출력하는 매크로
  printf("Print buffer:\n  >> "); \  // 버퍼 출력 시작 문구를 출력
  for (int i=0; i<len; i++) { \  // 0부터 len-1까지 반복하며 각 바이트를 출력
    printf("%02x ", p[i]); \  // p[i] 값을 2자리 16진수 형식으로 출력
    if (i % 16 == 15) printf("\n  >> "); \  // 16바이트마다 줄을 바꿔 보기 좋게 출력
  } \  // for 반복문을 종료
  printf("\n");  // 마지막 줄바꿈을 출력

#endif /* __BYTE_OP_H__ */  // 헤더 가드를 종료
