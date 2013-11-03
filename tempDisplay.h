#ifndef TEMPDISPLAY
#define TEMPDISPLAY

#include <stdio.h>
#include <stdlib.h>
#include <mysql.h> 
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <math.h>
#include <time.h>
#include <stdbool.h>

#include <wiringPi.h>
#include <wiringPiSPI.h>
#include "../lib/lib_mcp3002.h"
#include "../lib/lib_mpl115a2.h"
#include "../lib/lib_ST7032i.h"



#define MYSQL_SERVER "127.0.0.1" 
#define MYSQL_USERNAME "root" 
#define MYSQL_PASSWORD "bk3019" 
#define MYSQL_DB "tempDB"
#define MYSQL_SOCKET "/var/run/mysqld/mysqld.sock"
#define MYSQL_OPT 0
#define GREENPIN 3
#define REDPIN 5
#define BLUEPIN 4


#define LCD_ADDRESS  (0b0111110)
#define MPL115A2_ADR (0b1100000)
//プロトタイプ宣言
void insertRecordTemperatureTable(MYSQL *mysql,int id,char *datetime,double temp,double pres);
void getRecord(MYSQL *conn);
void getLastRecordGPIO(MYSQL *conn, bool *gpio22,bool *gpio23,bool *gpio24);

//スレッドループ処理：LED
void *thread_ledLoop( void *ptr );
void *thread_DBLoop( void *ptr );
pthread_mutex_t mutex;
pthread_cond_t cond;

//グローバル変数
MYSQL *mysql=0;

#endif
