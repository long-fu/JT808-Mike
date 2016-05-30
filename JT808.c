#include "JT808.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <mysql.h>

#define CARNUMBER 10000
#define SERVER "localhost"
#define DATABASE "JT808"
#define USERNAME "root"
#define PASSWORD ""
#define PORT "3306"

struct st_dataset dataarray[CARNUMBER];
char *name;
double lat_tran, lng_tran;


WORD order = 15;

// see interface for details
/*void *JT808_bodyppt(WORD bodylen, WORD encrypt, WORD sp, struct st_DataBodyProperty *result){
    result->BodyLen = bodylen;
    result->EncryptionType = encrypt;
    result->SplitBody = sp;
    result->Reserve = 0x00;
};*/

// see interface for details
struct st_DataHeader *JT808_header(WORD msgID, WORD bodylen, WORD encrypt, WORD sp, WORD order, BYTE *PhoneNum){
    struct st_DataHeader *output = malloc(sizeof(struct st_DataHeader));
    int length = strlen(PhoneNum);
    BYTE *phonewith0 = malloc(12*sizeof(BYTE));
    int i,a;
    for (i = 0; i < 12-length; i++){
        phonewith0[i] = '0';
    }
    for (a = 0; a <= length; a++, i++){
        phonewith0[i] = PhoneNum[a];
    }

    output->MsgID = msgID;
    output->Property.BodyLen = bodylen;
    output->Property.EncryptionType = encrypt;
    output->Property.SplitBody = sp;
    sscanf(phonewith0,"%02x%02x%02x%02x%02x%02x"
           ,(unsigned int *)&output->Phone[0],(unsigned int *)&output->Phone[1],
           (unsigned int *)&output->Phone[2],(unsigned int *)&output->Phone[3],
           (unsigned int *)&output->Phone[4],(unsigned int *)&output->Phone[5]);
    output->Order = order;
    free(phonewith0);
    order++;
    return output;
    }



// see interface for details
WORD JT808_body_array(struct st_GPS *gps, BYTE *Array, WORD arrlen){
    memset(Array, 0, arrlen*sizeof(BYTE));
    // bit0-bit3 GPS alart
    Array[0] = gps->alart >> 24;
    Array[1] = (gps->alart&0xff0000) >> 16;
    Array[2] = (gps->alart&0x00ff00) >> 8;
    Array[3] = (gps->alart&0x0000ff);
    // bit4-bit7 status
    Array[4] = gps->status >> 24;
    Array[5] = (gps->status&0xff0000) >> 16;
    Array[6] = (gps->status&0x00ff00) >> 8;
    Array[7] = (gps->status&0x0000ff);
    // bit8-bit11 latitude
    Array[8] = gps->lat >> 24;
    Array[9] = (gps->lat&0xff0000) >> 16;
    Array[10] = (gps->lat&0x00ff00) >> 8;
    Array[11] = (gps->lat&0x0000ff);
    // bit12-bit15 longitude
    Array[12] = gps->lng >> 24;
    Array[13] = (gps->lng&0xff0000) >> 16;
    Array[14] = (gps->lng&0x00ff00) >> 8;
    Array[15] = (gps->lng&0x0000ff);
    // bit16, bit17 altitude
    Array[16] = gps->altitude >> 8;
    Array[17] = (gps->altitude&0x00ff);
    // bit18, bit19 speed
    Array[18] = gps->speed >> 8;
    Array[19] = (gps->speed&0x00ff);
    // bit20 direction
    Array[20] = gps->direction;
    // bit21-bit26 time
    Array[21] = gps->GPStime[0];
    Array[22] = gps->GPStime[1];
    Array[23] = gps->GPStime[2];
    Array[24] = gps->GPStime[3];
    Array[25] = gps->GPStime[4];
    Array[26] = gps->GPStime[5];
    return arrlen;
    }


// see interface for details
WORD JT808_head_array(struct st_DataHeader *head, BYTE *Array, WORD arrlen){
    memset(Array, 0, arrlen*sizeof(BYTE));
    // bit0-bit1 Message ID
    Array[0] = head->MsgID >> 8;
    Array[1] = (head->MsgID&0x00ff);
    // bit2-bit3 Property
    WORD sum = (head->Property.Reserve << 14) + (head->Property.SplitBody << 13)
     + (head->Property.EncryptionType << 10) + (head->Property.BodyLen);
    Array[2] = sum >> 8;
    Array[3] = (sum&0x00ff);
    // bit4-bit9 Phone Number
    Array[4] = head->Phone[0];
    Array[5] = head->Phone[1];
    Array[6] = head->Phone[2];
    Array[7] = head->Phone[3];
    Array[8] = head->Phone[4];
    Array[9] = head->Phone[5];
    // bit10-bit11 Order
    Array[10] = head->Order >> 8;
    Array[11] = (head->Order&0x00ff);
    // bit12-bit16 Split
    Array[12] = head->SplitSign.Total >> 8;
    Array[13] = (head->SplitSign.Total&0x00ff);
    Array[14] = head->SplitSign.PkgOrder >> 8;
    Array[15] = (head->SplitSign.PkgOrder&0x00ff);
    return arrlen;
}

// see interface for details
BYTE *combine_array(BYTE *array1, BYTE *array2, BYTE *resultarr){
    WORD len1 = (sizeof(array1)/sizeof(BYTE));
    WORD len2 = (sizeof(array2)/sizeof(BYTE));
    memset(resultarr, 0, (len1+len2)/sizeof(BYTE));
    WORD i,a;
    for(i = 0; i < len1; i++){
        resultarr[i]=array1[i];
        }
    for(a = 0; a < len1; a++){
        resultarr[i]=array2[a];
    }
    return resultarr;
    }

// see interface for details
BYTE get_crc(BYTE *Array){
    WORD i;
    BYTE result;
    WORD arrlen = (sizeof(Array)/sizeof(BYTE));
    for(i = 0; i < arrlen; i++){
        result = Array[i] ^ result;
    }
    return result;
    }

// see interface for details
int JT808_convert(BYTE *arr){
    int len = 0;
    int i,a;
    for (i = 0; i < len; i++){
        if (arr[i]==0x7d){
            realloc(arr, sizeof(arr)+1);
            len++;
            for (a = len; a > i + 1; a--){
                arr[a] = arr[a-1];
            }
            arr[i+1] = 0x01;
        }
        else if (arr[i]==0x7e){
            realloc(arr, sizeof(arr)+1);
            len++;
            for (a = len; a > i + 1; a--){
                arr[a] = arr[a-1];
            }
            arr[i] = 0x7d;
            arr[i+1] = 0x02;
        }
    }
    return len;
}

// see interface for details
WORD JT808_final_array(BYTE *head, BYTE *body, BYTE *Array, WORD arrlen){
    memset(Array, 0, arrlen*sizeof(BYTE));
    int headlen, bodylen, i, a;
    Array[0] = 0x7e;

    headlen = 16 + JT808_convert(head);
    for (i = 0, a = 1; i < headlen; i++, a++){
        Array[a] = head[i];
    }
    JT808_convert(body);
    bodylen = 27 + JT808_convert(body);
    for (i = 0; i < bodylen; i++, a++){
        Array[a] = body[i];
    }
    BYTE *arr = malloc(100);
    arr = combine_array(head, body, arr);
    Array[a] = get_crc(arr);
    Array[a+1] = 0x7e;
    free(arr);
    return 3+headlen+bodylen;
}

void JT808_gpsinfo (BYTE *alart, BYTE *status, BYTE *lat, BYTE *lng, BYTE *speed, BYTE *direction, BYTE *altitude,
                                   BYTE *time, struct st_GPS *result)
    {
    sscanf(alart, "%x", &result->alart);
    sscanf(status, "%x", &result->status);
    sscanf(lat, "%lf", &lat_tran);
    result->lat = lat_tran * 1000000;
    sscanf(lng, "%lf", &lng_tran);
    result->lng = lng_tran * 1000000;
    sscanf(speed, "%d", &result->speed);
    result->speed = result->speed * 10;
    sscanf(direction, "%d", &result->direction);
    sscanf(altitude, "%d", &result->altitude);
    sscanf(time, "20%02x-%02x-%02x %02x:%02x:%02x",
           (unsigned int *)&result->GPStime[0],
           (unsigned int *)&result->GPStime[1],
           (unsigned int *)&result->GPStime[2],
           (unsigned int *)&result->GPStime[3],
           (unsigned int *)&result->GPStime[4],
           (unsigned int *)&result->GPStime[5]);
    }


void conn_sql(void){
    MYSQL_RES *res;
    MYSQL my_connection;
    MYSQL_ROW row;
    int t,i;
    i = 0;
    char *query;
    mysql_init(&my_connection);
    if (mysql_real_connect(&my_connection, SERVER, USERNAME, PASSWORD, DATABASE, PORT, NULL, 0)){
        printf("Connect successfully!\n");
        query = "select * from Car\n";
        t = mysql_real_query(&my_connection,query,(unsigned int) strlen(query));
        printf("%s\n",query);
        if (t != 0){
            printf("query error\n");
        } else {
            printf("query succeed\n");
            res = mysql_store_result(&my_connection);
            while(row = mysql_fetch_row(res)){
                        dataarray[i].phonenumber = row[0];
                        dataarray[i].alart = row[1];
                        dataarray[i].status = row[2];
                        dataarray[i].lat = row[3];
                        dataarray[i].lng = row[4];
                        dataarray[i].altitude = row[5];
                        dataarray[i].speed = row[6];
                        dataarray[i].direction = row[7];
                        dataarray[i].GPStime = row[8];
                    i++;
            }
 //       printf("mysql_free_result.../n");
 //           mysql_free_result(res);}
    printf("Phone:%s alart:%s status:%s latitude:%s longitude:%s altitude:%s speed:%s direction:%s time:%s\n"
           ,dataarray[0].phonenumber,dataarray[0].alart,dataarray[0].status,dataarray[0].lat,dataarray[0].lng,
           dataarray[0].altitude,dataarray[0].speed,dataarray[0].direction,dataarray[0].GPStime);
    mysql_free_result(res);

        }
    } else {
        printf("Connection failed!\n");
        mysql_close(&my_connection);
        exit(0);
        }
    }



int main(void){
    conn_sql();
    struct st_DataHeader *header_struct = malloc(sizeof(struct st_DataHeader));
    struct st_DataBodyProperty *property_struct = malloc(sizeof(struct st_DataBodyProperty));
    header_struct = JT808_header(0x0200,10,0,0,999,dataarray[0].phonenumber);
    //struct st_Message *testmessage = malloc(sizeof(struct st_Message));
    BYTE headary[50];
    JT808_head_array(header_struct, headary, 50);
    BYTE bodyary[50];
    struct st_GPS *result = malloc(sizeof(struct st_GPS));
    JT808_gpsinfo(dataarray[0].alart, dataarray[0].status, dataarray[0].lat, dataarray[0].lng, dataarray[0].speed,
                  dataarray[0].direction, dataarray[0].altitude, dataarray[0].GPStime, result);
    JT808_body_array(result, bodyary, 50);
    BYTE finalarr[100];
    int length = 38;
    length = JT808_final_array(headary, bodyary, finalarr, 100);
    int count = 0;
    while (count < length){
        printf("%02x ", finalarr[count]);
        count++;
    }
    free(result);
    free(header_struct);
    free(property_struct);
    return 1;
}
