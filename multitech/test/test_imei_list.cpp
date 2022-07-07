#include "gtest/gtest.h"

#include "imei_list.h"

#define time_offset_s 100
#define timestamp_imeiList 1652713247
#define new_imei 48830209
#define new_imei2 48820291

TEST( GivenAnImeiList, WhenImeiIsNew_ThenNotFoundInImeiList ) {
    // ARRANGE
    Imei_list imei_list;

    // ACT
    int result = imei_list.found_imei( new_imei );

    // ASSERT
    EXPECT_EQ( IMEI_NOT_FOUND, result );
};

TEST( GivenAnImeiList, WhenImeiIsNotNew_ThenFoundInImeiList ) {
    // ARRANGE
    Imei_list imei_list;
    uint16_t imei_pos = 0;

    // ACT
    imei_list.add_imei( new_imei );
    int result = imei_list.found_imei( new_imei );

    // ASSERT
    EXPECT_EQ( imei_pos, result );
};

TEST( GivenAnImeiList, WhenImeiIsNewAndUpdateTimestamp_ThenImeiIsAddedInList ) {
    // ARRANGE
    Imei_list imei_list;
    uint16_t imei_pos  = 0;

    // ACT
    imei_list.update_imei_timestamp( new_imei, timestamp_imeiList );
    int result = imei_list.found_imei( new_imei );

    // ASSERT
    EXPECT_EQ( imei_pos, result );
};

TEST( GivenAnImeiList, WhenAddMoreImeiThanImeiListMaxLen_ThenNotFoundInImeiList ) {
    // ARRANGE
    Imei_list imei_list;

    // ACT
    for ( uint8_t i = 0; i < ( UINT8_MAX - 1 ); i++ ) {
        imei_list.add_imei( new_imei );
    }
    bool result = imei_list.add_imei( new_imei2 );

    // ASSERT
    EXPECT_FALSE( result );
};

TEST( GivenAnImeiList, WhenImeiIsNew_ThenCheckImeiTimeFilterIsTrue ) {
    // ARRANGE
    Imei_list imei_list;
    uint32_t now_timestamp = timestamp_imeiList + time_offset_s;

    // ACT
    bool result = imei_list.check_send_pkt_by_satellite( new_imei, now_timestamp, time_offset_s );

    // ASSERT
    EXPECT_TRUE( result );
};

TEST( GivenAnImeiList, WhenAddImeiAndNewTimestampIsLessThanTimeOffset_ThenCheckImeiTimeFilterIsFalse ) {
    // ARRANGE
    Imei_list imei_list;
    uint32_t now_timestamp = timestamp_imeiList + 1;

    // ACT
    imei_list.add_imei( new_imei, timestamp_imeiList );
    bool result = imei_list.check_send_pkt_by_satellite( new_imei, now_timestamp, time_offset_s );

    // ASSERT
    EXPECT_FALSE( result );
};


TEST( GivenAnImeiList, WhenAddImeiAndNewTimestampIsMoreThanTimeOffset_ThenCheckImeiTimeFilterIsTrue ) {
    // ARRANGE
    Imei_list imei_list;
    uint32_t now_timestamp = timestamp_imeiList + 1 + time_offset_s;

    // ACT
    imei_list.add_imei( new_imei, timestamp_imeiList );
    bool result = imei_list.check_send_pkt_by_satellite( new_imei, now_timestamp, time_offset_s );

    // ASSERT
    EXPECT_TRUE( result );
};

TEST( GivenAnImeiList, WhenUpdateTimestamp_ThenCheckImeiTimeFilterIsFalse ) {
    // ARRANGE
    Imei_list imei_list;
    uint32_t now_timestamp = timestamp_imeiList + 1;

    // ACT
    imei_list.add_imei( new_imei );
    imei_list.update_imei_timestamp( new_imei, timestamp_imeiList );
    bool result = imei_list.check_send_pkt_by_satellite( new_imei, now_timestamp, time_offset_s );

    // ASSERT
    EXPECT_FALSE( result );
};