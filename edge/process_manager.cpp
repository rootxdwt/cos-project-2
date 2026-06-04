#include "process_manager.h"
#include "opcode.h"
#include "byte_op.h"
#include "setting.h"
#include <cstring>
#include <iostream>
#include <ctime>
using namespace std;

ProcessManager::ProcessManager()
{
  this->num = 0;
}

void ProcessManager::init()
{
}

// TODO: You should implement this function if you want to change the result of the aggregation
uint8_t *ProcessManager::processData(DataSet *ds, int *dlen)  // 데이터셋 ds를 가공해서 NetworkManager로 보낼 바이트 배열을 만드는 함수
{
  uint8_t *ret, *p;  // 반환할 메모리 ret과, 메모리 저장 위치를 이동시키기 위한 포인터 p를 선언
  int num, len;  // 데이터 개수와 길이 정보를 저장할 정수형 변수를 선언
  HouseData *house;  // 각 집의 데이터를 가리킬 포인터를 선언
  Info *info;  // 정보 데이터를 가리킬 포인터를 선언
  TemperatureData *tdata;  // 온도 데이터를 가리킬 포인터를 선언
  HumidityData *hdata;  // 습도 데이터를 가리킬 포인터를 선언
  PowerData *pdata;  // 전력 데이터를 가리킬 포인터를 선언
  char buf[BUFLEN];  // 임시 버퍼를 선언
  ret = (uint8_t *)malloc(BUFLEN);  // BUFLEN 크기만큼 메모리를 동적 할당하고, uint8_t 포인터로 형변환하여 ret에 저장
  int tmp, min_humid, min_temp, min_power, month;  // 임시 값, 최소 습도, 최소 온도, 최소 전력, 월 정보를 저장할 변수를 선언
  time_t ts;  // timestamp 값을 저장할 time_t 변수를 선언
  struct tm *tm;  // timestamp를 날짜/시간 구조체로 변환해서 저장할 포인터를 선언

  tdata = ds->getTemperatureData();  // 데이터셋 ds에서 온도 데이터 객체를 가져옴
  hdata = ds->getHumidityData();  // 데이터셋 ds에서 습도 데이터 객체를 가져옴
  num = ds->getNumHouseData();  // 데이터셋 ds에서 집 데이터의 개수를 가져옴

  // Example) I will give the minimum daily temperature (1 byte), the minimum daily humidity (1 byte), 
  // the minimum power data (2 bytes), the month value (1 byte) to the network manager
  
  // Example) getting the minimum daily temperature
  min_temp = (int) tdata->getMin();  // 온도 데이터에서 최솟값을 가져와 int형으로 변환한 뒤 min_temp에 저장

  // Example) getting the minimum daily humidity
  min_humid = (int) hdata->getMin();  // 습도 데이터에서 최솟값을 가져와 int형으로 변환한 뒤 min_humid에 저장

  // Example) getting the minimum power value
  min_power = 10000;  // 최소 전력 값을 찾기 위해 초기값을 10000으로 설정
  for (int i=0; i<num; i++)  // 모든 집 데이터를 하나씩 확인하기 위해 반복문을 실행
  {
    house = ds->getHouseData(i);  // i번째 집 데이터를 가져
    pdata = house->getPowerData();  // 해당 집의 전력 데이터 객체를 가져
    tmp = (int)pdata->getValue();  // 전력 값을 가져와 int형으로 변환한 뒤 tmp에 저장

    if (tmp < min_power)  // 현재 전력 값 tmp가 지금까지의 최소 전력 값보다 작으면
      min_power = tmp;  // 최소 전력 값을 tmp로 갱신
  }

  // Example) getting the month value from the timestamp
  ts = ds->getTimestamp();  // 데이터셋 ds에서 timestamp 값을 가져
  tm = localtime(&ts);  // timestamp 값을 로컬 시간 기준의 tm 구조체로 변환
  month = tm->tm_mon + 1;  // tm_mon은 0부터 시작하므로 1을 더해서 실제 월 값으로 변환

  // Example) initializing the memory to send to the network manager
  memset(ret, 0, BUFLEN);  // ret 메모리 공간을 0으로 초기화
  *dlen = 0;  // 전송할 데이터 길이를 0으로 초기화
  p = ret;  // p가 ret의 시작 주소를 가리키게 함

  // Example) saving the values in the memory
  VAR_TO_MEM_1BYTE_BIG_ENDIAN(min_temp, p);  // 최소 온도 값을 1바이트 big-endian 형식으로 메모리에 저장
  *dlen += 1;  // 저장한 데이터 길이에 1바이트를 더함
  VAR_TO_MEM_1BYTE_BIG_ENDIAN(min_humid, p);  // 최소 습도 값을 1바이트 big-endian 형식으로 메모리에 저장
  *dlen += 1;  // 저장한 데이터 길이에 1바이트를 더함
  VAR_TO_MEM_2BYTES_BIG_ENDIAN(min_power, p);  // 최소 전력 값을 2바이트 big-endian 형식으로 메모리에 저장
  *dlen += 2;  // 저장한 데이터 길이에 2바이트를 더함
  VAR_TO_MEM_1BYTE_BIG_ENDIAN(month, p);  // 월 값을 1바이트 big-endian 형식으로 메모리에 저장
  *dlen += 1;  // 저장한 데이터 길이에 1바이트를 더함

  return ret;  // 가공이 끝난 바이트 배열의 시작 주소를 반환
}
