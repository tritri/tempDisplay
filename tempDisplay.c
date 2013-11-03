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


int main(int argc, char **argv)
{
  int lcd;       // ファイルディスクリプタ:LCD
  int pres;      //ファイルディスクリプタ:MPL115A2（大気圧センサ）
  char *i2cFileName = "/dev/i2c-1"; // I2Cドライバファイル名
  char strDisp[100];
  char strDate[256];
  int lcdAddress = LCD_ADDRESS;  // I2C LCD のアドレス。
  int presAddress = MPL115A2_ADR;//圧力センサのアドレス
  time_t current_time,before_time;//時間計測用構造体
  double sec_time;//時間（秒）
  struct tm *date;
  int idCounter;
  bool lcdFlag;

  int i;
  if(argc>1){
    if(strcmp("-h",argv[1])==0){
      printf("データロギングプログラムbyTritri\n引数\n-lcdon:液晶表示を行います \n");
      exit(1);
    }else if(strcmp("-lcdon",argv[1])==0){
      lcdFlag=true;
      printf("LCD On!!!\n");
    }else{
      lcdFlag=false;
      printf("LCD Off!!\n");
    }
  }else{
    lcdFlag=false;
      printf("LCD Off!!\n");
}

//GPIOセットアップ(LED点灯)
  if (wiringPiSetup() == -1)
    return 1;
  printf("Wiringpi setupOK\n");

  pinMode(GREENPIN, OUTPUT);
  pinMode(REDPIN, OUTPUT);
  pinMode(BLUEPIN, OUTPUT);
  digitalWrite(BLUEPIN, 0); 
  digitalWrite(GREENPIN, 0); 
  digitalWrite(REDPIN, 0);
  
  if(lcdFlag){
    // I2CポートをRead/Write属性でオープン。ファイルディスクリプタ取得:lcd
    if ((lcd = open(i2cFileName, O_RDWR)) < 0)
      {
	printf("Faild to open i2c port:lcd\n");
	exit(1);
      }
   
    // 通信先アドレスの設定。:LCD
    if (ioctl(lcd, I2C_SLAVE, lcdAddress) < 0)
      {
	printf("Unable to get lcd bus access to talk to slave\n");
	exit(1);
      }
    delay(100);
  }else{
  
    // I2CポートをRead/Write属性でオープン、ファイルディスクリプタ取得。:圧力センサ
  if ((pres = open(i2cFileName, O_RDWR)) < 0)
    {
      printf("Faild to open i2c port:pres\n");
      exit(1);
    }
  // 通信先アドレスの設定。:圧力センサ
  if (ioctl(pres,I2C_SLAVE,presAddress) < 0)
    {
      printf("Unable to get pres bus access to talk to slave\n");
      exit(1);
    }
  delay(100);
  
  }

if(lcdFlag){
    // LCD初期化。
    LCD_init(lcd);
 
    // メッセージ表示。
    LCD_setCursor(0, 0, lcd);
    LCD_puts("Temperature", lcd);
  
    printf("Lcd Output OK!\n");
  }else{
    double f;
    f=calcPressure(pres);
    printf("Pressure(hPa)=%f\n",f);    
 }

  /*
  //LCD表示
  if (wiringPiSetup() == -1)
    return 1;
  printf("setupOK\n");
  */

  int ans;
  ans=wiringPiSPISetup (channel, 1000000);
  if (ans < 0)
  {
    fprintf (stderr, "SPI Setup failed: %s\n", strerror (errno));
  }else
    {
      printf("Spi setup OK!!!\n");
    }

  char str[100];
 
  
  //int i;
  //for (i=1; i<=10; i++) {
  //   sprintf(str,"ループ回数：%2d",i);
  //  puts(str);
  //}
 


  dispStatusTemp(0,str);

  puts(str);
  printf("disptempOK\n");

  
//ここから---------------------------------------
//MYSQL *mysql=0;
MYSQL_RES *result;
MYSQL_ROW record; 

MYSQL mysql_buf;

//MySqlのDBへの接続
mysql_init(&mysql_buf); //← MySQL 構造体の初期化mysql_init関数
mysql = mysql_real_connect(&mysql_buf, //← MySQLデータベースにログインmysql_real_connect関数
MYSQL_SERVER, MYSQL_USERNAME,
MYSQL_PASSWORD, MYSQL_DB, MYSQL_PORT,
MYSQL_SOCKET, MYSQL_OPT);
 

if (!mysql) //← 変数mysqlには接続確立時はMYSQL*接続ハンドル,非確立時はNULLが入る。
{
  printf("failed to connect to mysql server (server=%s, userid=%s)\n",
	 MYSQL_SERVER ? MYSQL_SERVER : "",
	 MYSQL_USERNAME ? MYSQL_USERNAME : "");
  return (-1);
}
//ここまで----------------------------------------------------



 printf("thread Start\n");  
  //スレッド処理
  pthread_t thread1, thread2;
  useconds_t tick1 = 200000;
  pthread_create( &thread1, NULL, thread_ledLoop, (void *) &tick1);     
  pthread_create( &thread2, NULL, thread_DBLoop, (void *) &pres);     
  pthread_join( thread1, NULL);
  pthread_join( thread2, NULL);
  pthread_mutex_destroy(&mutex);


 /*//以下単純にポーリングでの処理
 bool gpio22=true;
 bool gpio23=true;
 bool gpio24=true;
 double tmpTemperature;
 double tmpPressure;
 idCounter=0;
 time(&before_time);
 
 int lightLux=0;
 

  //ループ
  while (1)
    {
      if(lcdFlag){
	sprintf(strDisp,"%f",getTemperature(0));
	//printf("Temperature= %f(℃)",getTemperature(0));
	//printf("%s", *dispStatusTemp(0));
	LCD_setCursor(5, 1, lcd);
	LCD_puts(strDisp, lcd);
      }
      //mysqlレコード追加
      time(&current_time);
      sec_time=difftime(current_time,before_time);
      if(sec_time>1.0)
      {
	tmpTemperature=getTemperature(0);
	tmpPressure=calcPressure(pres);
	before_time=current_time;
	//printf("\n%f(s)",sec_time);
	idCounter++;
	date=localtime(&current_time);
	strftime(strDate,255,"%Y-%m-%d %H:%M:%S",date);
	printf("%s Temp(℃)%f Pressure(hPa)%f\n",strDate,tmpTemperature,tmpPressure);
	insertRecordTemperatureTable(mysql,idCounter,strDate,tmpTemperature,tmpPressure);    
      }
      //LED点灯
      getLastRecordGPIO(mysql,&gpio22,&gpio23,&gpio24);

      digitalWrite(BLUEPIN, gpio22); 
      digitalWrite(GREENPIN, gpio23); 
      digitalWrite(REDPIN, gpio24);
      
      if(lightLux>1024){
	lightLux=0;
      }else{
	lightLux++;
      }
      //printf("%d\n",lightLux);
      int i;
      for(i=0;i<1024;i++){
	if(i >lightLux){
	  digitalWrite(REDPIN, false); 
	}else{
	  digitalWrite(REDPIN, true); 
	}

      }
    }
  */
  return 0;
}

//DBアクセス用スレッドループ
//1sごとのポーリングを行います
void *thread_DBLoop(void *ptr){
  int pres = *( int * )ptr;//ファイルディスクリプタ:MPL115A2（大気圧センサ）
  bool gpio22=true;
  bool gpio23=true;
  bool gpio24=true;
  double tmpTemperature;
  double tmpPressure;
  int idCounter=0;
  time_t current_time,before_time;//時間計測用構造体
  double sec_time;//時間（秒）
  struct tm *date;
  char strDate[256];
  
  //スリープ時間を指定
  struct timespec ts;
  ts.tv_sec=1;//1sを指定
  ts.tv_nsec=0;//0nsを指定

  time(&before_time);
  while(1){
  
        //mysqlレコード追加
      time(&current_time);
      sec_time=difftime(current_time,before_time);
      if(sec_time>60.0)
      {
	tmpTemperature=getTemperature(0);
	tmpPressure=calcPressure(pres);
	before_time=current_time;
	//printf("\n%f(s)",sec_time);
	idCounter++;
	date=localtime(&current_time);
	strftime(strDate,255,"%Y-%m-%d %H:%M:%S",date);
	printf("%s Temp(℃)%f Pressure(hPa)%f\n",strDate,tmpTemperature,tmpPressure);
	
	pthread_mutex_lock(&mutex);
	insertRecordTemperatureTable(mysql,idCounter,strDate,tmpTemperature,tmpPressure);
	pthread_mutex_unlock(&mutex);
    
      }
      //LED点灯
      pthread_mutex_lock(&mutex);
      getLastRecordGPIO(mysql,&gpio22,&gpio23,&gpio24);
      pthread_mutex_unlock(&mutex);
      
      digitalWrite(BLUEPIN, gpio22); 
      digitalWrite(GREENPIN, gpio23); 
      digitalWrite(REDPIN, gpio24);
      nanosleep(&ts,NULL);//1msスリープ
  }
}
void *thread_ledLoop(void *ptr){
  useconds_t tick = *( int * )ptr;
  int lightLux=0;
  //スリープ時間を指定
  struct timespec ts;
  ts.tv_sec=0;//1sを指定
  ts.tv_nsec=1000000;//0nsを指定
 
  while(1){
      if(lightLux>1024){
	lightLux=0;
      }else{
	lightLux++;
      }
      //printf("%d\n",lightLux);
      int i,j;
      for(j=0;j<4;j++){
	for(i=0;i<1024;i++){
	  if(i >lightLux){
	    digitalWrite(REDPIN, false); 
	  }else{
	    digitalWrite(REDPIN, true); 
	  }
	
	}
      }
      nanosleep(&ts,NULL);//1msスリープ

  }
}
void getLastRecordGPIO(MYSQL *conn, bool *gpio22,bool *gpio23,bool *gpio24)
{

  MYSQL_RES *res;
  MYSQL_ROW row;
  //最新のレコードを取得
  if (mysql_query(conn, "SELECT * FROM GPIO ORDER BY id desc limit 1")) {
    fprintf(stderr, "%s\n", mysql_error(conn));
    exit(0);
  }

  res = mysql_use_result(conn);

  //レコードを文字列として整形
 char str22[255];
 char str23[255];
 char str24[255];
 
 while ((row = mysql_fetch_row(res)) != NULL) {
   //printf("%s,%s,%s, %s, %s\n",
   //	   row[0],row[1], row[2], row[3],row[4]);
    sprintf(str22,"%s",row[2]);
    sprintf(str23,"%s",row[3]);
    sprintf(str24,"%s",row[4]);
  }
 mysql_free_result(res);
 //文字列をboolに変換
  if(strcmp(str22,"0"))
  {
    *gpio22=true; 
  }else{
    *gpio22=false;
  }

  if(strcmp(str23,"0"))
  {
    *gpio23=true; 
  }else{
    *gpio23=false;
  }
  if(strcmp(str24,"0"))
  {
    *gpio24=true; 
  }else{
    *gpio24=false;
  }
  /*  
if(*gpio==true)
  {
    printf("true\n");
  }else{ 
  printf("false\n");
  }*/
}
void getRecord(MYSQL *conn)
{

  MYSQL_RES *res;
  MYSQL_ROW row;

  if (mysql_query(conn, "SELECT * FROM GPIO limit 30")) {
    fprintf(stderr, "%s\n", mysql_error(conn));
    exit(0);
  }
  res = mysql_use_result(conn);
  while ((row = mysql_fetch_row(res)) != NULL) {
    printf("%s,%s,%s, %s, %s\n",
	   row[0],row[1], row[2], row[3],row[4]);
    
  }
}
//temperatureテーブルへレコードを書き込みます
//id：ユニークな値
//datetime：日時の文字列（例"2013-1-1 1:33:22")
//temp：温度
void insertRecordTemperatureTable(MYSQL *mysql,int id,char *datetime,double temp,double pres)
{
	char query[256];
	memset(query,'\0',256);
	/*tableへの書き込み*/
	//int id=4;
	//char *strDatetime;
	//strDatetime="2013-1-1 1:33:22";
	//double temp=13.3333;
	//datetime型のinsertにはcast関数を使用すること
	//またidはユニークな値しか許されない

	//idを自分で計算する場合に使用
	//sprintf(query,"insert into temperature(id,datetime,temp) values(%1d,cast('%2s' as datetime),%3f)",id,datetime,temp);
	//idをオートインクリメントに設定したテーブルで使用
	sprintf(query,"insert into temperature2(datetime,temp,pressure) values(cast('%1s' as datetime),%2f,%2f)",datetime,temp,pres);

	printf(query);
	printf("\n");
	mysql_query(mysql,query);
}
