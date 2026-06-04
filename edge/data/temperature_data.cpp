#include "temperature_data.h"

TemperatureData::TemperatureData(time_t timestamp, double min, double max, double avg) : Data(timestamp, avg, "celcius")
{
  this->min = min;
  this->max = max;
  this->next = NULL;
}

void TemperatureData::setNext(TemperatureData *next)
{
  this->next = next;
}

TemperatureData *TemperatureData::getNext()
{
  return this->next;
}

void TemperatureData::setMin(double min)
{
  this->min = min;
}

double TemperatureData::getMin()
{
  return this->min;
}

void TemperatureData::setMax(double max)
{
  this->max = max;
}

double TemperatureData::getMax()
{
  return this->max;
}
