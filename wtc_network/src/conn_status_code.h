#pragma once

namespace Conn_status
{
    typedef enum { 
        cipstatus_code_inital         = 0,
        cipstatus_code_attaching      = 1,
        cipstatus_code_attached       = 2,
        cipstatus_code_connecting     = 3,
        cipstatus_code_connected      = 4,
        cipstatus_code_remote_closing = 5,
        cipstatus_code_closing        = 6,
        cipstatus_code_closed         = 7,
        cipstatus_code_unattached     = 8
    } cipstatus_code_t;
};
