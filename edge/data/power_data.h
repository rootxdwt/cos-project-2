#ifndef __POWER_DATA_H__
#define __POWER_DATA_H__

#include <random>
#include <ctime>
#include <string>
#include "info.h"
#include "data.h"

class PowerData : public Data
{
  private:
    PowerData *next;
  public:
    PowerData(time_t timestamp, double avg);

    void setNext(PowerData *data);
    PowerData *getNext();
};

#endif /* __POWER_DATA_H__ */
