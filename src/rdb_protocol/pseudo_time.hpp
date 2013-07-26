// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_PSEUOD_TIME_HPP_
#define RDB_PROTOCOL_PSEUDO_TIME_HPP_

#include <boost/date_time.hpp>

#include "rdb_protocol/env.hpp"

namespace ql {
namespace pseudo {
extern const char *const time_string;

counted_t<const datum_t> iso8601_to_time(const std::string &s, const rcheckable_t *t);
std::string time_to_iso8601(counted_t<const datum_t> d);
double time_to_epoch_time(counted_t<const datum_t> d);

counted_t<const datum_t> time_now();
counted_t<const datum_t> time_in_tz(counted_t<const datum_t> t,
                                    counted_t<const datum_t> tz);

int time_cmp(const datum_t& x, const datum_t& y);
void rcheck_time_valid(const datum_t *time);
counted_t<const datum_t> make_time(double epoch_time, std::string tz="");
counted_t<const datum_t> time_add(
    counted_t<const datum_t> x, counted_t<const datum_t> y);
counted_t<const datum_t> time_sub(
    counted_t<const datum_t> x, counted_t<const datum_t> y);

enum time_component_t {
    YEAR,
    MONTH,
    DAY,
    DAY_OF_WEEK,
    DAY_OF_YEAR,
    HOURS,
    MINUTES,
    SECONDS
};
double time_portion(counted_t<const datum_t> time, time_component_t c);
counted_t<const datum_t> time_date(counted_t<const datum_t> time,
                                   const rcheckable_t *target);

} //namespace pseudo
} //namespace ql

#endif //RDB_PROTOCOL_PSEUDO_TIME_HPP_
