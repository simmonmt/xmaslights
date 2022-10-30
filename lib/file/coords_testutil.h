#ifndef _LIB_FILE_COORDS_TESTUTIL_H_
#define _LIB_FILE_COORDS_TESTUTIL_H_ 1

#include "gtest/gtest.h"
#include "lib/file/coords.h"

::testing::Matcher<std::tuple<CoordsRecord, CoordsRecord>> CoordsRecordEq(
    double final_within);

::testing::Matcher<CoordsRecord> CoordsRecordEq(const CoordsRecord& want,
                                                double final_within);

#endif  // _LIB_FILE_COORDS_TESTUTIL_H_
