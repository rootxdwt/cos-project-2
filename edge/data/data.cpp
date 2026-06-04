#include "data.h"

Data::Data(time_t timestamp, double avg, string unit)
{
  this->timestamp = timestamp;
  this->avg = avg;
  this->unit = unit;
}

void Data::setValue(double value)
{
  this->avg = value;
}

double Data::getValue()
{
  return this->avg;
}

void Data::setTimestamp(time_t timestamp)
{
  this->timestamp = timestamp;
}

time_t Data::getTimestamp()
{
  return this->timestamp;
}

string Data::getUnit()
{
  return this->unit;
}
