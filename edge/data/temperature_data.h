#ifndef __TEMPERATURE_H__
#define __TEMPERATURE_H__

#include <random>
#include <ctime>
#include <string>
#include "info.h"
#include "data.h"

class TemperatureData : public Data
{
  private:
    double min;
    double max;
    TemperatureData *next;
  public:
    TemperatureData(time_t timestamp, double min, double max, double avg);

    void setNext(TemperatureData *data);
    TemperatureData *getNext();

    void setMin(double min);
    double getMin();

    void setMax(double max);
    double getMax();
};

#endif /* __TEMPERATURE_H__ */
