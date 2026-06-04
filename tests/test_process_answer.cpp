#include "../edge/setting.h"
#include "../edge/edge.h"

#include <iostream>
#include <ctime>

#include "../edge/byte_op.h"

#define BUFLEN 1024

using namespace std;

int main(int argc, char *argv[]) // 프로그램의 메인 함수 시작
{ // 메인 함수 바디 시작
  DataReceiver *dr; // 데이터 수집기 객체를 가리킬 포인터 변수 선언
  DataSet *ds; // 가구 및 환경 데이터 통합셋 객체를 가리킬 포인터 변수 선언
  HouseData *house; // 개별 가구 정보 객체를 가리킬 포인터 변수 선언
  TemperatureData *tdata; // 기온 통계 데이터 객체를 가리킬 포인터 변수 선언
  HumidityData *hdata; // 습도 통계 데이터 객체를 가리킬 포인터 변수 선언
  PowerData *pdata; // 전력 소모 데이터 객체를 가리킬 포인터 변수 선언
  int num, tmp; // 가구 수(num) 및 형변환/임시 계산용 변수(tmp) 선언
  time_t curr, ts; // 데이터 조회용 시간(curr) 및 패킷 타임스탬프(ts) 변수 선언
  double max_temp, avg_temp, min_temp; // 기온의 최댓값, 평균값, 최솟값 저장 변수 선언
  double max_humid, avg_humid, min_humid; // 습도의 최댓값, 평균값, 최솟값 저장 변수 선언
  double power, sum_power, avg_power, max_power, min_power; // 전력값 계산용 변수들 선언
  unsigned char buf[BUFLEN]; // 바이너리 패킷을 조립할 1024바이트 크기의 문자 배열 버퍼 선언
  unsigned char *p; // 버퍼 내부에서 데이터를 순차적으로 채우며 이동할 포인터 변수 선언

  curr = 1609459200; // 데이터 조회 기준 시점 설정 (UNIX 타임스탬프 기준 2021-01-01 00:00:00)
  dr = new DataReceiver(); // 데이터 수집기 객체를 동적 할당하여 인스턴스 생성
  ds = dr->getDataSet(curr); // 기준 시점에 해당하는 전체 데이터 세트를 가져와 ds에 저장
  
  // 1. Write a statement to get the timestamp value to 'ts' and print out the value (please refer to dataset.h)
  ts = ds->getTimestamp(); // 데이터 세트의 생성 타임스탬프 값을 읽어와 ts 변수에 저장
  cout << "timestamp: " << ts << endl; // 읽어온 타임스탬프 수치를 콘솔 창에 출

  // 2. Write a statement to get the number of house data that contains the private information and the power value to 'num' (dataset.h)
  num = ds->getNumHouseData(); // 데이터 세트에 포함된 총 가구의 개수를 구하여 num에 저장
  cout << "# of house data: " << num << endl; // 수집된 총 가구 수를 콘솔 창에 출력

  // 3. Write a statement to get the first house data to 'house' (please refer to dataset.h) 
  house = ds->getHouseData(0); // 데이터 세트에서 인덱스 0번인 '첫 번째' 가구 데이터를 가져옴
  
  // Write a statement to get the 10th house data to 'house' (dataset.h)
  house = ds->getHouseData(9); // 데이터 세트에서 인덱스 9번인 '열 번째' 가구 데이터를 가져와 house에 덮어씀
  
  // Get the power data to 'pdata' (house_data.h)
  pdata = house->getPowerData(); // 해당 가구의 전력 사용량 세부 데이터 객체 포인터를 pdata에 저장
  
  // Get the daily power value to 'power' and print out the value (power_data.h)
  power = pdata->getValue(); // 전력 데이터 객체로부터 실제 전력 소모량 수치(double)를 읽어와 저장
  cout << "Power: " << power << endl; // 읽어온 개별 가구의 전력 값을 콘솔 창에 출력
  
  // Explicitly cast the type from double to int and assign it to 'tmp', and print out the value
  tmp = (int)pdata->getValue(); // 실수형(double) 전력 값을 정수형(int)으로 명시적 형변환하여 tmp에 대입
  cout << "Power (casted): " << tmp << endl; // 소수점이 없어진 정수형 전력 값을 콘솔 창에 출력
  
  // Compute the value averaged over all the power data by using 'sum_power' and 'num', 
  // assign the average value to 'avg_power', and print out the value
  sum_power = 0; // 전체 가구의 전력 합산을 계산하기 위해 누적 변수를 0으로 초기화
  for (int i=0; i<num; i++) // 수집된 총 가구 수(num)만큼 루프를 돌며 전력 값을 누적 시작
  { // 전력 합산 루프 시작
    house = ds->getHouseData(i); // i번째 인덱스 가구의 데이터를 순차적으로 참조
    pdata = house->getPowerData(); // 해당 가구의 전력 데이터 객체 가져오기
    sum_power += pdata->getValue(); // 가구별 전력 소모량을 읽어와 sum_power 변수에 누적 합산
  } // 전력 합산 루프 끝
  avg_power = sum_power / num; // 총 합산 전력량을 총 가구 수로 나누어 평균 전력 소모량 계산
  cout << "Power (avg): " << avg_power << endl; // 계산된 평균 전력 사용량을 콘솔 창에 출력
  
  // Find the maximum value among all the power data 
  max_power = -1; // 최댓값 비교를 위해 시스템에서 나올 수 없는 가장 작은 수(-1)로 초기화
  for (int i=0; i<num; i++) // 모든 가구를 순회하며 최댓값을 찾기 위한 반복문 수행 시작
  { // 전력 최댓값 검색 루프 시작
    house = ds->getHouseData(i); // i번째 인덱스의 가구 데이터 참조
    pdata = house->getPowerData(); // 해당 가구의 전력 데이터 객체 획득
    power = pdata->getValue(); // 가구의 실제 전력 소모값(double) 읽기

    if (power > max_power) // 현재 가구의 전력이 기존에 기록된 최댓값보다 크다면 조건문 실행
      max_power = power; // 현재 가구의 전력값을 새로운 최댓값으로 갱신
  } // 전력 최댓값 검색 루프 끝
  cout << "Power (max): " << max_power << endl; // 전체 가구 중 가장 높은 전력 소모값을 콘솔 창에 출력
  
  // Find the minimum value among all the power data
  min_power = 10000; // 최솟값 비교를 위해 충분히 큰 상한선 수치(10000)로 변수 초기화
  for (int i=0; i<num; i++) // 모든 가구를 순회하며 최솟값을 찾기 위한 반복문 수행 시작
  { // 전력 최솟값 검색 루프 시작
    house = ds->getHouseData(i); // i번째 인덱스의 가구 데이터 참조
    pdata = house->getPowerData(); // 해당 가구의 전력 데이터 객체 획득
    power = pdata->getValue(); // 가구의 실제 전력 소모값 읽기

    if (power < min_power) // 현재 가구의 전력이 기존에 기록된 최솟값보다 작다면 조건문 실행
      min_power = power; // 현재 가구의 전력값을 새로운 최솟값으로 갱신
  } // 전력 최솟값 검색 루프 끝
  cout << "Power (min): " << min_power << endl; // 전체 가구 중 가장 낮은 전력 소모값을 콘솔 창에 출력

  // 4. Write a statement to get the temperature data to 'tdata' (dataset.h)
  tdata = ds->getTemperatureData(); // 데이터 세트로부터 통합 일일 기온 데이터 객체 포인터를 tdata에 저장
  
  // Get the maximum value of the daily temperature (temperature_data.h)
  max_temp = tdata->getMax(); // 일일 최고 기온 수치(double)를 읽어와 max_temp에 대입
  cout << "Temperature (max): " << max_temp << endl; // 최고 기온 값을 콘솔 창에 출력
  
  // Get the average value of the daily temperature (temperature_data.h)
  avg_temp = tdata->getValue(); // 일일 평균 기온 수치를 읽어와 avg_temp에 대입
  cout << "Temperature (avg): " << avg_temp << endl; // 평균 기온 값을 콘솔 창에 출력
  
  // Get the minimum value of the daily temperature (temperature_data.h)
  min_temp = tdata->getMin();  // 일일 최저 기온 수치를 읽어와 min_temp에 대입
  cout << "Temperature (min): " << min_temp << endl; // 최저 기온 값을 콘솔 창에 출력
  
  // Explicitly cast the type of the maximum value from double to int, assign the resultant value to 'tmp', and print it out
  tmp = (int)tdata->getMax(); // 최고 기온 소수점을 버리고 정수형으로 명시적 캐스팅하여 tmp에 저장
  cout << "Temperature (max, casted): " << tmp << endl; // 정수형으로 변환된 최고 기온 값을 콘솔 창에 출력

  // 5. Write a statement to get the humidity data to 'hdata' (dataset.h)
  hdata = ds->getHumidityData(); // 데이터 세트로부터 통합 일일 습도 데이터 객체 포인터를 hdata에 저장
  
  // Get the maximum value of the daily humidity (humidity_data.h)
  max_humid = hdata->getMax(); // 일일 최고 습도 수치(double)를 읽어와 max_humid에 대입
  cout << "Humidity (max): " << max_humid << endl; // 최고 습도 값을 콘솔 창에 출력
  
  // Get the average value of the daily humidity (humidity_data.h)
  avg_humid = hdata->getValue(); // 일일 평균 습도 수치를 읽어와 avg_humid에 대입
  cout << "Humidity (avg): " << avg_humid << endl; // 평균 습도 값을 콘솔 창에 출력
  
  // Get the minimum value of the daily humidity (humidity_data.h)
  min_humid = hdata->getMin(); // 일일 최저 습도 수치를 읽어와 min_humid에 대입
  cout << "Humidity (min): " << min_humid << endl; // 최저 습도 값을 콘솔 창에 출력
  
  // Explicitly cast the type of the minimum value from double to int, assign the resultant value to 'tmp', and print it out
  tmp = (int)hdata->getMin(); // 최저 습도 소수점을 자르고 정수형으로 캐스팅하여 tmp에 저장
  cout << "Humidity (min, casted): " << tmp << endl; // 정수형으로 변환된 최저 습도 값을 콘솔 창에 출력

  // 6. Initialize the buffer 'buf' with zeros (its length is defined as BUFLEN) (use the memset() function, please google it!)
  memset(buf, 0, BUFLEN); // 패킷 조립용 버퍼(buf) 전체를 0으로 깨끗하게 채워 초기화 수행

  // 7. Write statements to save the values into 'buf' using 'p' as follows:
  // # of house data (2 bytes) || maximum power (integer) (4 bytes) || maximum temperature (integer) (2 bytes)
  // Print out the buffer
  // Please use the macros defined in edge/byte_op.h
  p = buf; // 포인터 p가 바이너리 적재를 위해 버퍼(buf)의 맨 첫 주소를 가리키도록 설정
  VAR_TO_MEM_2BYTES_BIG_ENDIAN(num, p); // 가구 수(num)를 2바이트 빅엔디안 바이너리로 버퍼에 적재 후 포인터 p 이동
  tmp = (int)min_power; // 실수형 최저 전력을 정수로 바꾸어 임시 변수에 대입 
  VAR_TO_MEM_4BYTES_BIG_ENDIAN(tmp, p); // 4바이트 빅엔디안 바이너리로 변환하여 버퍼에 적재 후 포인터 p 이동
  tmp = (int)max_temp; // 최고 기온 수치를 정수형으로 변환하여 임시 변수에 대입
  VAR_TO_MEM_4BYTES_BIG_ENDIAN(tmp, p); // 4바이트 빅엔디안으로 적재 

  tmp = p - buf; // 데이터가 채워지며 이동한 최종 포인터 주소에서 시작 주소를 빼 패킷의 총 바이트 길이 계산
  PRINT_MEM(buf, tmp); // 매크로 함수를 사용하여 버퍼(buf)에 생성된 패킷 바이너리 데이터를 16진수 형태로 화면에 덤프 출력

	return 0; // 프로그램이 아무런 오류 없이 완벽하게 정상 종료되었음을 운영체제에 알림(0 반환)
} // 메인 함수 바디 끝
