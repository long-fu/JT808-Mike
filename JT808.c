#include "JT808.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <mysql.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <syslog.h>
#include <time.h>




#define NewPath "/tmp/JT808_Log"

struct st_dataset dataarray[10000];
static int global_order = 0;
FILE *logfp, *cfgfp;
int CarNumber, MySQLPort, PacketEncryption, PacketSplit, PacketType, TCPPort, Interval;
char MySQLServer[50], MySQLDatabase[50], MySQLUsername[50], MySQLPassword[50], TCPServer[50], TargetID[50], MySQLTable[50];


char *ConfigFilter (char *src)
{
    int length = strlen(src);
    if (length == 0)
    {
        return src;
    }
    char *tmp = malloc(length-1);
    int i,u;
    for (i = 0, u = 0; i < length; i++)
    {
        if (src[i]!=34)
        {
            tmp[u]=src[i];
            u++;
        }
    }
    tmp[u] = '\0';
    return tmp;
}

void write_log(const char *pFilename, FILE *fp, const char *pMessageinfo){
    time_t t;
    if((fp=fopen(pFilename,"a")) >=0){
        time(&t);
        fprintf(fp, "%s - ", asctime(localtime(&t)));
      fprintf(fp, pMessageinfo);
      fclose(fp);
    }
}

int Read_CfgFile(const char *CfgName, FILE *fp)
    {
        int ArgumentsRead = 0;
        if ((fp = fopen(CfgName,"rt")) >= 0)
        {
            memset(MySQLServer, 0, 50);
            memset(MySQLDatabase, 0, 50);
            memset(MySQLUsername, 0, 50);
            memset(MySQLPassword, 0, 50);
            memset(TCPServer, 0, 50);

        ArgumentsRead +=fscanf(fp, "Carnumber=%d\n", &CarNumber);
        ArgumentsRead +=fscanf(fp, "Encryption=%d\n", &PacketEncryption);
        ArgumentsRead +=fscanf(fp, "Split= %d\n", &PacketSplit);
        ArgumentsRead +=fscanf(fp, "PacketType=%x\n", &PacketType);
        ArgumentsRead +=fscanf(fp, "MySQLPort=%d\n", &MySQLPort);
        ArgumentsRead +=fscanf(fp, "MySQLServer=%s\n", MySQLServer);
        ArgumentsRead +=fscanf(fp, "MySQLDatabase=%s\n", MySQLDatabase);
        ArgumentsRead +=fscanf(fp, "MySQLUsername=%s\n", MySQLUsername);
        ArgumentsRead +=fscanf(fp, "MySQLPassword=%s\n", MySQLPassword);
        ArgumentsRead +=fscanf(fp, "TCPServer=%s\n", TCPServer);
        ArgumentsRead +=fscanf(fp, "TCPPort=%d\n", &TCPPort);
        ArgumentsRead +=fscanf(fp, "TargetID=%s\n", TargetID);
        ArgumentsRead +=fscanf(fp, "Interval=%d\n",&Interval);
        ArgumentsRead +=fscanf(fp, "MySQLTable=%s",MySQLTable);

     /*   printf("AR:%d\n",ArgumentsRead);
        printf("CN:%d\n",CarNumber);
        printf("EN:%d\n",PacketEncryption);
        printf("SP:%d\n",PacketSplit);
        printf("TYPE:%x\n",PacketType);
        printf("MYSQLPORT:%d\n",MySQLPort);
        printf("MYSQLSERVER:%s\n",MySQLServer);
        printf("TCPSERVER:%s\n",TCPServer);
        printf("TCPPORT:%d\n",TCPPort);
        printf("MYSQLDB:%s\n",MySQLDatabase);
        printf("MYSQLUSR:%s\n",MySQLUsername);
         printf("MYSQLPWD:%s\n",MySQLPassword);*/
        }
        fclose(fp);
        return ArgumentsRead;
    }

// see interface for details
void JT808_header(WORD msgID, WORD bodylen, WORD encrypt, WORD sp, WORD order, BYTE *PhoneNum, struct st_DataHeader *output){

    //memset(output, 0, sizeof(struct st_DataHeader));
    int length = strlen((char*)PhoneNum);
    BYTE phonewith0[12];
    int i,a;
    for (i = 0; i < 12-length; i++){
        phonewith0[i] = '0';
    }
    for (a = 0; a <= length; a++, i++){
        phonewith0[i] = PhoneNum[a];
    }
    output->Property.SplitBody = sp;
    output->MsgID = msgID;
    output->Property.BodyLen = bodylen;
    output->Property.EncryptionType = encrypt;

    sscanf((char*)phonewith0,"%02x%02x%02x%02x%02x%02x"
           ,(unsigned int *)&output->Phone[0],(unsigned int *)&output->Phone[1],
           (unsigned int *)&output->Phone[2],(unsigned int *)&output->Phone[3],
           (unsigned int *)&output->Phone[4],(unsigned int *)&output->Phone[5]);
    output->Order = order;

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
    // bit20, bit21 direction
    Array[20] = gps->direction >> 8;
    Array[21] = (gps->direction&0x00ff);
    // bit22-bit27 time
    Array[22] = gps->GPStime[0];
    Array[23] = gps->GPStime[1];
    Array[24] = gps->GPStime[2];
    Array[25] = gps->GPStime[3];
    Array[26] = gps->GPStime[4];
    Array[27] = gps->GPStime[5];
    arrlen = 28;
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
    if (head->Property.SplitBody == 1){
    Array[12] = head->SplitSign.Total >> 8;
    Array[13] = (head->SplitSign.Total&0x00ff);
    Array[14] = head->SplitSign.PkgOrder >> 8;
    Array[15] = (head->SplitSign.PkgOrder&0x00ff);
    arrlen = 16;
    } else {
    arrlen = 12;
    }
    return arrlen;
}

// see interface for details
void *combine_array(BYTE *array1, BYTE *array2, int Array1_Length, int Array2_Length, BYTE *InputArray)
{
    WORD i,a;
    for(i = 0; i < Array1_Length; i++){
        InputArray[i]=array1[i];
        }
    for(a = 0; a < Array2_Length; a++, i++){
       InputArray[i]=array2[a];
    }
    return 0;
}

// see interface for details
BYTE get_crc(BYTE *Array, int ArrayLength){
    WORD i;
    BYTE result = 0;

    for(i = 0; i < ArrayLength; i++){
        result = result ^ Array[i];
    }
    return result;
    }

// see interface for details
int JT808_convert(BYTE *arr){
    int len = 0;
    int i,a;
    for (i = 0; i < len; i++){
        if (arr[i]==0x7d){
  //          realloc(arr, sizeof(arr)+1);
            len++;
            for (a = len; a > i + 1; a--){
                arr[a] = arr[a-1];
            }
            arr[i+1] = 0x01;
        }
        else if (arr[i]==0x7e){
   //         realloc(arr, sizeof(arr)+1);
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
WORD JT808_final_array(BYTE *head, BYTE *body, BYTE *finalary, WORD HeaderLength, WORD BodyLength, WORD arrlen){
    memset(finalary, 0, arrlen*sizeof(BYTE));
    int i, a, extralength;
    finalary[0] = 0x7e;
    for (i = 0, a = 1; i < HeaderLength; i++, a++){
        finalary[a] = head[i];
    }
    for (i = 0; i < BodyLength; i++, a++){
        finalary[a] = body[i];
    }
    BYTE temp[1000];
    combine_array(head, body, HeaderLength, BodyLength, temp);
    finalary[a] = get_crc(temp, HeaderLength+BodyLength);
    if (finalary[a] == 0x7d){
        finalary[a+1] = 0x01;
        finalary[a+2] = 0x7e;
        extralength = 4;
    } else if (finalary[a] == 0x7e){
        finalary[a] = 0x7d;
        finalary[a+1] = 0x02;
        finalary[a+2] = 0x7e;
        extralength = 4;
        } else {
            finalary[a+1] = 0x7e;
            extralength = 3;
        }
   // free(temp);
    return extralength+BodyLength+HeaderLength;
}

void JT808_gpsinfo (BYTE *alart, BYTE *status, BYTE *lat, BYTE *lng, BYTE *speed, BYTE *direction, BYTE *altitude,
                                   BYTE *time, struct st_GPS *result)
    {
    double lat_tran, lng_tran;
    sscanf((char*)alart, "%x", (unsigned int*)&result->alart);
    sscanf((char*)status, "%x", (unsigned int*)&result->status);
    sscanf((char*)lat, "%lf", &lat_tran);
    result->lat = lat_tran * 1000000;
    sscanf((char*)lng, "%lf", &lng_tran);
    result->lng = lng_tran * 1000000;
    sscanf((char*)speed, "%d", (int*)&result->speed);
    result->speed = result->speed * 10;
    sscanf((char*)direction, "%d", (int*)&result->direction);
    sscanf((char*)altitude, "%d", (int*)&result->altitude);
    sscanf((char*)time, "20%02x-%02x-%02x %02x:%02x:%02x",
           (unsigned int *)&result->GPStime[0],
           (unsigned int *)&result->GPStime[1],
           (unsigned int *)&result->GPStime[2],
           (unsigned int *)&result->GPStime[3],
           (unsigned int *)&result->GPStime[4],
           (unsigned int *)&result->GPStime[5]);
    }


WORD conn_sql(char *server, char *usr, char *pwd, char *db, int port, char *table, char *target){
    MYSQL_RES *res;
    MYSQL my_connection;
    MYSQL_ROW row;
    int t,i,TotalNumber;
    char query[500];
    memset(query, 0, 500);
    mysql_init(&my_connection);
    if (mysql_real_connect(&my_connection, server, usr, pwd, db, port, NULL, 0)){
        write_log(NewPath, logfp, "Connect Database successfully!\n");
     sprintf(query,"select CarID,0,3,LastLat,LastLon,90,LastSpeed,LastHeading,LastGPSDate from Car where LastGPSDate>date_sub(now(),INTERVAL %d SECOND) and BookUserList like '%%%%%s%%%%' limit 100\n",30,target);

        write_log(NewPath, logfp, query);
        t = mysql_real_query(&my_connection,query,(unsigned int) strlen(query));
        if (t != 0){
        write_log(NewPath, logfp, "Query Error\n");
        } else {
            write_log(NewPath, logfp, "Query Succeed\n");
            res = mysql_store_result(&my_connection);
            TotalNumber = mysql_num_rows(res);
            i = 0;
            while((row = mysql_fetch_row(res))){
                        dataarray[i].phonenumber = (BYTE*)row[0];
                        dataarray[i].alart = (BYTE*)row[1];
                        dataarray[i].status = (BYTE*)row[2];
                        dataarray[i].lat = (BYTE*)row[3];
                        dataarray[i].lng = (BYTE*)row[4];
                        dataarray[i].altitude = (BYTE*)row[5];
                        dataarray[i].speed = (BYTE*)row[6];
                        dataarray[i].direction = (BYTE*)row[7];
                        dataarray[i].GPStime = (BYTE*)row[8];
                        dataarray[i].order = global_order;
                    global_order++;
                    i++;
                    //sprintf(query,"update JT808INFO set HaveRead=1 where ID=%s\n",row[9]);
                    //write_log("JT808_Log", fp ,query);
                    //t = mysql_real_query(&my_connection,query,(unsigned int) strlen(query));
            }
            mysql_free_result(res);
            mysql_close(&my_connection);
        }
    } else {
      write_log(NewPath, logfp, "MySQL Server Connection failed!\n");
        mysql_close(&my_connection);
        exit(0);
        }
        return TotalNumber;
    }

int SocketWrite(int nSocket, char *pBuffer, int nLen, int nTimeout){
    int nOffset = 0;
    int nWrite;
    int nLeft=nLen;
    int nLoop=0;
    int nTotal=0;
    int nNewTimeout=nTimeout*10;
    //char buffer[60];
    while ((nLoop<=nNewTimeout)&&(nLeft>0)){
        nWrite = send(nSocket, pBuffer+nOffset, nLeft, 0);
        if (nWrite==0){
            return -1;
        }
        if (nWrite==-1){
            if(errno!=EAGAIN){
                return -1;
            }
        }
        if (nWrite<0){
            return nWrite;
        }
        nOffset+=nWrite;
        nLeft-=nWrite;
        nTotal+=nWrite;
        if (nLeft>0){
            usleep(100000);
        }
        nLoop++;
    }
    //sprintf(buffer, "%d Packets have been sent!\n", nWrite);
    //write_log(NewPath,logfp, buffer);
   // free(buffer);
    return nWrite;
    }

/*int SocketRead(int nSocket, char *pBuffer, int nLen, int flags){

    int nRead;

    //int nNewTimeout=nTimeout*10;
    memset(pBuffer, 0, 4096);
    nRead = recv(nSocket, pBuffer, 4096, 0);
    sleep(1);
    return nRead;
    }*/

int conn_TCP_Server(char *ipaddr, int tcp_port){
    signal(SIGTTOU,SIG_IGN);
    signal(SIGPIPE,SIG_IGN);
    signal(SIGALRM,SIG_IGN);
    struct hostent *pHost = NULL;
    struct hostent localHost;
    char pHostData[4096];
    char LogMessage[100];
    int h_errorno=0;
    int h_rc=gethostbyname_r(ipaddr, &localHost, pHostData, 4096, &pHost, &h_errorno);
    if ((pHost == 0)||(h_rc!=0)){
        write_log(NewPath, logfp, "TCP Error!\n");
        return 0;
    }
    int s = 0;
    s = socket(AF_INET, SOCK_STREAM, 0);

    if (s < 0){
       write_log(NewPath, logfp, "Socket Error!\n");
        return 0;
    }
    struct in_addr in;
    memcpy(&in.s_addr, pHost->h_addr_list[0], sizeof(in.s_addr));
    struct sockaddr_in name;
    memset(&name, 0, sizeof(struct sockaddr_in));
    name.sin_addr.s_addr = in.s_addr;
    name.sin_family =AF_INET;
    name.sin_port = htons((WORD)tcp_port);
    if (connect(s,(struct sockaddr *)&name,sizeof(name)) != 0){
        sprintf(LogMessage, "TCP Connection Problem! Error Code: %d\n", errno);
        write_log(NewPath, logfp, LogMessage);
        memset(LogMessage, 0, 100);
        return s;
    }
     long fileattr;
    fileattr = fcntl(s, F_GETFL);
    fcntl(s, F_SETFL, fileattr|O_NDELAY);
    write_log(NewPath, logfp, "TCP Connection Created!\n");
 /*   char ReadBuffer[4096];
    int RecvLen;
    while (1){
        RecvLen = SocketRead(s, ReadBuffer, 4096, 0);
        if (RecvLen > 0){
            printf("%s",ReadBuffer);
        }
    }*/
    return s;
    }

void init_deamon(void){
    int pid, i;
    if ((pid = fork())){
        exit(0);
    } else if (pid < 0){
        exit(1);
        }
    setsid();
     if ((pid = fork())){
        exit(0);
    } else if (pid < 0){
        exit(1);
        }
     chdir("/tmp");
    for (i = 0; i < NOFILE; i++){
        close(i);
    }
    umask(0);
    }


unsigned long get_file_size(const char *path)
{
    unsigned long filesize = -1;
    struct stat statbuff;
    if(stat(path, &statbuff) < 0){
        return filesize;
    }else{
        filesize = statbuff.st_size;
    }
    return filesize;
}

int main(void){
    int headerlength, bodylength, packetlength, z, CarInServer, s;
    char ReceiveBuffer[4096];
    if ((Read_CfgFile("JT808Cfg", cfgfp)) != 14)
    {
        write_log(NewPath, logfp, "Cannot Read Config File!\n");
        exit(0);
    } else {
    write_log(NewPath, logfp, "Read Config File Successfully!\n");
    }
    char *NewPWD = ConfigFilter(MySQLPassword);
    char *NewTCP = ConfigFilter(TCPServer);
    char *NewSQL = ConfigFilter(MySQLServer);
    char *NewDB = ConfigFilter(MySQLDatabase);
    char *NewUSR = ConfigFilter(MySQLUsername);
    char *NewTarget = ConfigFilter(TargetID);
    char *NewTable = ConfigFilter(MySQLTable);
    struct st_DataHeader *header_struct  = malloc(sizeof(struct st_DataHeader));
    struct st_GPS *result = malloc(sizeof(struct st_GPS));
    daemon(0,0);
    //init_deamon();
    s = conn_TCP_Server(NewTCP, TCPPort);
    while (1){

        CarInServer = conn_sql(NewSQL,NewUSR,NewPWD,NewDB,MySQLPort,NewTable,NewTarget);
        for (z = 0; z < CarInServer; z++){
        //printf("Phone:%s alart:%s status:%s latitude:%s longitude:%s altitude:%s speed:%s direction:%s time:%s\n"
        //      ,dataarray[z].phonenumber,dataarray[z].alart,dataarray[z].status,dataarray[z].lat,dataarray[z].lng,
        //      dataarray[z].altitude,dataarray[z].speed,dataarray[z].direction,dataarray[z].GPStime);
        BYTE headary[50];
        BYTE bodyary[50];
        BYTE finalary[100];
        JT808_gpsinfo(dataarray[z].alart, dataarray[z].status, dataarray[z].lat, dataarray[z].lng, dataarray[z].speed,
                    dataarray[z].direction, dataarray[z].altitude, dataarray[z].GPStime, result);
        bodylength = JT808_body_array(result, bodyary, 50) + JT808_convert(bodyary);
        JT808_header(PacketType,bodylength,PacketEncryption,PacketSplit,dataarray[z].order,dataarray[z].phonenumber,header_struct);
        headerlength = JT808_head_array(header_struct, headary, 50) + JT808_convert(headary);
        packetlength = JT808_final_array(headary, bodyary, finalary, headerlength, bodylength, 100);
        //for (count = 0; count < packetlength; count++){
        //    printf("%02x ", finalary[count]);
        //    }
           while ((SocketWrite(s,(char*)finalary,packetlength,5)) < 1){
              write_log(NewPath, logfp, "Disconnected...Reconnecting in 5 seconds...\n");
                shutdown(s, SHUT_RDWR);
                close(s);
                sleep(5);
                s = conn_TCP_Server(NewTCP,TCPPort);
        }

        //DataReceived = recv(s, ReceiveBuffer, 4096, 0);
        //DataSent = SocketWrite(s,"pulse",6,5);
        write_log(NewPath, logfp, (const char *)dataarray[z].phonenumber);

        memset(headary, 0, sizeof(headary));
        memset(bodyary, 0, sizeof(bodyary));
        memset(finalary, 0, sizeof(finalary));
        memset(ReceiveBuffer, 0, sizeof(ReceiveBuffer));
        //sleep(1);
        }
        unsigned long filesize = get_file_size(NewPath);
        if (filesize > 21000000){  // About 20MB
            logfp = fopen(NewPath, "w+");
            fclose(logfp);
            write_log(NewPath, logfp, "Log Cleared!");
        }
    sleep(Interval);

    }
    header_struct = NULL;
        result = NULL;
        free(result);
        free(header_struct);
    return 1;
}
