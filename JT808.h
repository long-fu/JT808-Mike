#ifndef JT808_H_INCLUDED
#define JT808_H_INCLUDED
// define data types which would be used by the client
typedef unsigned short WORD;
typedef unsigned long DWORD;
typedef unsigned char BYTE;



struct st_dataset{
    BYTE *phonenumber;
    BYTE *alart;
    BYTE *status;
    BYTE *lat;
    BYTE *lng;
    BYTE *altitude;
    BYTE *speed;
    BYTE *direction;
    BYTE *GPStime;
    DWORD order;
    };

struct st_DataBodyProperty{
    WORD BodyLen:10;
    WORD EncryptionType:3;
    WORD SplitBody:1;
    WORD Reserve:2;
    };

struct st_Split{
    WORD Total;
    WORD PkgOrder;
    };

struct st_DataHeader{
    WORD MsgID;
    struct st_DataBodyProperty Property;
    BYTE Phone[6];
    WORD Order;
    struct st_Split SplitSign;
    };

/*struct st_Message{
    BYTE Head;
    BYTE *MessageHead;
    BYTE *GPSinfo;
    BYTE CRC;
    BYTE Tail;
    };*/



struct st_GPS{
    DWORD alart;
    DWORD status;
    DWORD lat;
    DWORD lng;
    WORD altitude;
    WORD speed;
    WORD direction;
    BYTE GPStime[6];
    };


// *JT808_header(WORD msgID, WORD msgPpt, BYTE PhoneNum) is passed
// a Message ID, a DataBodyProperty and a Phone Number.  It
// returns a pointer to a DataHeader where includes all the inputs in given formats.
// efficiency: O(n)
void JT808_header(WORD msgID, WORD bodylen, WORD encrypt, WORD sp, WORD order, BYTE *PhoneNum, struct st_DataHeader *output);

// *JT808_bodyppt(WORD bodylen, WORD encrypt, WORD sp, struct st_DataBodyProperty *result) is passed
// a Message, a Message Encryption Type, a number that determines
// if the message needs to be splited(1->need split) and a array. It
// returns a pointer to a DataBodyProperty where includes all the inputs in given formats.
// efficiency: O(n)
void *JT808_bodyppt(WORD bodylen, WORD encrypt, WORD sp, struct st_DataBodyProperty *result);

// *JT808_struct_to_array(struct st_Message *msg) is passed a pointer to a Message
// and returns a pointer to an array where stores all the information in the Message.
WORD JT808_body_array(struct st_GPS *gps, BYTE *Array, WORD arrlen);

WORD JT808_head_array(struct st_DataHeader *head, BYTE *Array, WORD arrlen);

void *combine_array(BYTE *array1, BYTE *array2, int Array1_Length, int Array2_Length, BYTE *InputArray);

BYTE get_crc(BYTE *Array, int ArrayLength);

WORD JT808_final_array(BYTE *head, BYTE *body, BYTE *finalary, WORD HeaderLength, WORD BodyLength, WORD arrlen);


#endif // JT808_H_INCLUDED
