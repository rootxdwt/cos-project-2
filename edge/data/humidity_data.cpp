#include "humidity_data.h"

HumidityData::HumidityData(time_t timestamp, double min, double max, double avg) : Data(timestamp, avg, "\%")
{
  this->min = min;
  this->max = max;
  this->next = NULL;
}

void HumidityData::setNext(HumidityData *next)
{
  this->next = next;
}

HumidityData *HumidityData::getNext()
{
  return this->next;
}

void HumidityData::setMin(double min)
{
  this->min = min;
}

double HumidityData::getMin()
{
  return this->min;
}

void HumidityData::setMax(double max)
{
  this->max = max;
}

double HumidityData::getMax()
{
  return this->max;
}
