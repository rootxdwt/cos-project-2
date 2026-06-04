#ifndef __DATA_H__
#define __DATA_H__

#include <ctime>
#include <string>

using namespace std;

class Data
{
  protected:
    time_t timestamp;
    double avg;
    string unit;
  public:
    Data(time_t timestamp, double avg, string unit);
    virtual ~Data() {}

    void setValue(double value);
    double getValue();

    void setTimestamp(time_t timestamp);
    time_t getTimestamp();

    string getUnit();
};

#endif /* __DATA_H__ */
