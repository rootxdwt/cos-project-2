#ifndef __HUMIDITY_DATA_H__
#define __HUMIDITY_DATA_H__

#include <random>
#include <ctime>
#include <string>
#include "info.h"
#include "data.h"

class HumidityData : public Data
{
  private:
    double min;
    double max;
    HumidityData *next;
  public:
    HumidityData(time_t timestamp, double min, double max, double avg);

    void setNext(HumidityData *data);
    HumidityData *getNext();

    void setMin(double min);
    double getMin();

    void setMax(double max);
    double getMax();
};

#endif /* __HUMIDITY_DATA_H__ */
