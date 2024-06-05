#ifndef _TSD_ARIBTIME_HPP_
#define _TSD_ARIBTIME_HPP_

#include <chrono>
#include "util.hpp"

namespace tsd
{
using std::chrono::system_clock;

// BCD
struct aribduration
{
  uint8_t hour;
  uint8_t min;
  uint8_t sec;
public:
  void unpack(const char* data, size_t size) {
    const uint8_t* p = reinterpret_cast<const uint8_t*>(data);
    unpack(&p, p+size);
  }

  void unpack(const uint8_t** pp, const uint8_t* pend) {
    const uint8_t* p = *pp;
    if(pend - p < 3)
      std::runtime_error("");

    hour = bcd_to_decimal(get8(p));
    p += 1;
    min = bcd_to_decimal(get8(p));
    p += 1;
    sec = bcd_to_decimal(get8(p));
    p += 1;

    *pp = p;
  }

private:
  uint8_t bcd_to_decimal(uint8_t bcd) const {
    return (bcd >> 4)*10 + (bcd & 0x0F);
  }
};

struct aribtime
{
  uint16_t mjd;
  uint8_t hour;
  uint8_t min;
  uint8_t sec;
public:
  void unpack(const char* data, size_t size) {
    const uint8_t* p = reinterpret_cast<const uint8_t*>(data);
    unpack(&p, p+size);
  }

  void unpack(const uint8_t** pp, const uint8_t* pend) {
    const uint8_t* p = *pp;
    if(pend - p < 5)
      std::runtime_error("");

    mjd = get16(p);
    p += 2;

    aribduration bcd;
    bcd.unpack(&p, pend);
    hour = bcd.hour;
    min = bcd.min;
    sec = bcd.sec;

    *pp = p;
  }

  time_t to_time_t() const {
    return mjd_to_time_t(mjd) + hour*60*60 + min*60 + sec - 9*60*60;
  }

  system_clock::time_point time() const {
    return system_clock::from_time_t(to_time_t());
  }

private:
  time_t mjd_to_time_t(uint16_t mjd) const {
    return (mjd - 40587)*86400;
  }

};

}

#endif
