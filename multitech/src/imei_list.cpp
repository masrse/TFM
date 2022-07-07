#include "imei_list.h"

Imei_list::Imei_list() : imei_list_len( 0 ) {}

Imei_list::~Imei_list() {}

bool Imei_list::add_imei( uint32_t new_imei, uint32_t timestamp ) {
    if ( imei_list_len < imei_list_max_len ) {
        imei_list[imei_list_len].imei      = new_imei;
        imei_list[imei_list_len].timestamp = timestamp;
        imei_list_len++;
        return true;
    }
    return false;
}

int Imei_list::found_imei( uint32_t imei ) {
    for ( int8_t i = 0; i < imei_list_len; i++ ) {
        if ( imei == imei_list[i].imei ) {
            return i;
        }
    }
    return IMEI_NOT_FOUND;
}

void Imei_list::update_imei_timestamp( uint32_t imei, uint32_t timestamp ) {
    int imei_pos = found_imei( imei );
    if ( imei_pos == IMEI_NOT_FOUND ) {
        add_imei( imei, timestamp );
    }
    else {
        imei_list[imei_pos].timestamp = timestamp;
    }
}

bool Imei_list::check_send_pkt_by_satellite( uint32_t imei, uint32_t timestamp, uint32_t time_filter_s ) {
    int imei_pos = found_imei( imei );
    if ( imei_pos != IMEI_NOT_FOUND && ( timestamp < ( imei_list[imei_pos].timestamp + time_filter_s ) ) ) {
        return false;
    }
    return true;
}
