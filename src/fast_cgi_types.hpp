#pragma once


namespace cxxnever
{
namespace fastcgi
{

/*
 * Listening socket file number
 */
const int FCGI_LISTENSOCK_FILENO = 0;

struct FCGI_Header
{
    unsigned char version;
    unsigned char type;
    unsigned char requestIdB1;
    unsigned char requestIdB0;
    unsigned char contentLengthB1;
    unsigned char contentLengthB0;
    unsigned char paddingLength;
    unsigned char reserved;
};

/*
 * Number of bytes in a FCGI_Header.  Future versions of the protocol
 * will not reduce this number.
 */
const int FCGI_HEADER_LEN = 8;

/*
 * Value for version component of FCGI_Header
 */
const int FCGI_VERSION_1 = 1;

/*
 * Values for type component of FCGI_Header
 */
enum
{
    FCGI_BEGIN_REQUEST =      1,
    FCGI_ABORT_REQUEST =      2,
    FCGI_END_REQUEST =        3,
    FCGI_PARAMS =             4,
    FCGI_STDIN =              5,
    FCGI_STDOUT =             6,
    FCGI_STDERR =             7,
    FCGI_DATA =               8,
    FCGI_GET_VALUES =         9,
    FCGI_GET_VALUES_RESULT = 10,
    FCGI_UNKNOWN_TYPE =      11,
};

/*
 * Value for requestId component of FCGI_Header
 */
const int FCGI_NULL_REQUEST_ID = 0;

struct FCGI_BeginRequestBody
{
    unsigned char roleB1;
    unsigned char roleB0;
    unsigned char flags;
    unsigned char reserved[5];
};

struct FCGI_BeginRequestRecord
{
    FCGI_Header header;
    FCGI_BeginRequestBody body;
};

/*
 * Mask for flags component of FCGI_BeginRequestBody
 */
const int FCGI_KEEP_CONN = 1;

/*
 * Values for role component of FCGI_BeginRequestBody
 */
const int FCGI_RESPONDER =  1;
const int FCGI_AUTHORIZER = 2;
const int FCGI_FILTER =     3;

struct FCGI_EndRequestBody
{
    unsigned char appStatusB3;
    unsigned char appStatusB2;
    unsigned char appStatusB1;
    unsigned char appStatusB0;
    unsigned char protocolStatus;
    unsigned char reserved[3];
};

struct FCGI_EndRequestRecord {
    FCGI_Header header;
    FCGI_EndRequestBody body;
};

/*
 * Values for protocolStatus component of FCGI_EndRequestBody
 */
enum
{
    FCGI_REQUEST_COMPLETE = 0,
    FCGI_CANT_MPX_CONN =    1,
    FCGI_OVERLOADED =       2,
    FCGI_UNKNOWN_ROLE =     3,
};

/*
 * Variable names for FCGI_GET_VALUES / FCGI_GET_VALUES_RESULT records
 */
const char* FCGI_MAX_CONNS =  "FCGI_MAX_CONNS";
const char* FCGI_MAX_REQS =   "FCGI_MAX_REQS";
const char* FCGI_MPXS_CONNS = "FCGI_MPXS_CONNS";

struct FCGI_UnknownTypeBody
{
    unsigned char type;
    unsigned char reserved[7];
};

struct FCGI_UnknownTypeRecord
{
    FCGI_Header header;
    FCGI_UnknownTypeBody body;
};

}
}
