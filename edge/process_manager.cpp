#include "process_manager.h"
#include "opcode.h"
#include "byte_op.h"
#include "setting.h"
#include <cstring>
#include <iostream>
#include <ctime>
#include <cmath>
using namespace std;

ProcessManager::ProcessManager()
{
  this->num = 0;
}

void ProcessManager::init()
{
}

uint8_t *ProcessManager::processData(DataSet *ds, int *dlen)
{
  uint8_t *ret, *p;
  int num;
  HouseData *house;
  TemperatureData *tdata;
  HumidityData *hdata;
  PowerData *pdata;
  ret = (uint8_t *)malloc(BUFLEN);
  int month;
  time_t ts;
  struct tm *tm;

  tdata = ds->getTemperatureData();
  hdata = ds->getHumidityData();
  num = ds->getNumHouseData();

  // Basic weather metrics
  double t_avg = tdata->getValue();
  double h_avg = hdata->getValue();
  double max_temp = tdata->getMax();
  double min_temp_val = tdata->getMin();

  // 1. 불쾌지수
  double thi = 0.81 * t_avg + 0.01 * h_avg * (0.99 * t_avg - 14.3) + 46.3;

  // 2. 온도편차
  double temp_range = max_temp - min_temp_val;

  // Extract month from timestamp
  ts = ds->getTimestamp();
  tm = localtime(&ts);
  month = tm->tm_mon + 1;

  // Basic power metrics
  double sum_power = 0;
  int min_power = 1000000;
  int max_power = -1;

  for (int i = 0; i < num; i++)
  {
    house = ds->getHouseData(i);
    pdata = house->getPowerData();
    int val = (int)pdata->getValue();
    sum_power += val;
    if (val < min_power) min_power = val;
    if (val > max_power) max_power = val;
  }
  double avg_power = sum_power / num;

  // 3. 가구별 표준편차
  double sum_sq_diff_power = 0;
  for (int i = 0; i < num; i++)
  {
    house = ds->getHouseData(i);
    pdata = house->getPowerData();
    double diff = pdata->getValue() - avg_power;
    sum_sq_diff_power += diff * diff;
  }
  double std_dev_power = sqrt(sum_sq_diff_power / num);

  memset(ret, 0, BUFLEN);
  *dlen = 0;
  p = ret;

#if FEATURE_COMBINATION == 1
  // combination 1: dimension: 6, Buffer Size: 11
  // 0. avg_power (2 bytes)
  //
  // 1. avg_temp (2 bytes, scaled by 10)
  // 2. avg_humid (2 bytes, scaled by 10)
  // 3. month (1 byte)
  // 4. thi_val (2 bytes, scaled by 10)
  // 5. temp_range (2 bytes, scaled by 10)
  int t_avg_scaled = (int)(t_avg * 10);
  int h_avg_scaled = (int)(h_avg * 10);
  int thi_scaled = (int)(thi * 10);
  int temp_range_scaled = (int)(temp_range * 10);
  int avg_power_int = (int)avg_power;

  VAR_TO_MEM_2BYTES_BIG_ENDIAN(avg_power_int, p);
  *dlen += 2;
  VAR_TO_MEM_2BYTES_BIG_ENDIAN(t_avg_scaled, p);
  *dlen += 2;
  VAR_TO_MEM_2BYTES_BIG_ENDIAN(h_avg_scaled, p);
  *dlen += 2;
  VAR_TO_MEM_1BYTE_BIG_ENDIAN(month, p);
  *dlen += 1;
  VAR_TO_MEM_2BYTES_BIG_ENDIAN(thi_scaled, p);
  *dlen += 2;
  VAR_TO_MEM_2BYTES_BIG_ENDIAN(temp_range_scaled, p);
  *dlen += 2;

#elif FEATURE_COMBINATION == 2
  // combination 2: dimension: 5, Buffer Size: 10
  // 0. avg_power (2 bytes) - 일별 전체 가구 평균 전력사용량
  //
  // 1. min_power (2 bytes)
  // 2. max_power (2 bytes)
  // 3. std_dev_power (2 bytes)
  // 4. papr (2 bytes, scaled by 100)
  int std_dev_power_int = (int)std_dev_power;
  int avg_power_int = (int)avg_power;
  int papr_scaled = (int)((max_power / avg_power) * 100);

  VAR_TO_MEM_2BYTES_BIG_ENDIAN(avg_power_int, p);
  *dlen += 2;
  VAR_TO_MEM_2BYTES_BIG_ENDIAN(min_power, p);
  *dlen += 2;
  VAR_TO_MEM_2BYTES_BIG_ENDIAN(max_power, p);
  *dlen += 2;
  VAR_TO_MEM_2BYTES_BIG_ENDIAN(std_dev_power_int, p);
  *dlen += 2;
  VAR_TO_MEM_2BYTES_BIG_ENDIAN(papr_scaled, p);
  *dlen += 2;

#else
  // combination 3: dimension: 7, Buffer Size: 13
  // 0. avg_power (2 bytes)
  //
  // 1. thi_val (2 bytes, scaled by 10)
  // 2. temp_range (2 bytes, scaled by 10)
  // 3. month (1 byte)
  // 4. min_power (2 bytes)
  // 5. std_dev_power (2 bytes)
  // 6. papr (2 bytes, scaled by 100)
  int thi_scaled = (int)(thi * 10);
  int temp_range_scaled = (int)(temp_range * 10);
  int avg_power_int = (int)avg_power;
  int std_dev_power_int = (int)std_dev_power;
  int papr_scaled = (int)((max_power / avg_power) * 100);

  VAR_TO_MEM_2BYTES_BIG_ENDIAN(avg_power_int, p);
  *dlen += 2;
  VAR_TO_MEM_2BYTES_BIG_ENDIAN(thi_scaled, p);
  *dlen += 2;
  VAR_TO_MEM_2BYTES_BIG_ENDIAN(temp_range_scaled, p);
  *dlen += 2;
  VAR_TO_MEM_1BYTE_BIG_ENDIAN(month, p);
  *dlen += 1;
  VAR_TO_MEM_2BYTES_BIG_ENDIAN(min_power, p);
  *dlen += 2;
  VAR_TO_MEM_2BYTES_BIG_ENDIAN(std_dev_power_int, p);
  *dlen += 2;
  VAR_TO_MEM_2BYTES_BIG_ENDIAN(papr_scaled, p);
  *dlen += 2;

#endif

  return ret;
}
