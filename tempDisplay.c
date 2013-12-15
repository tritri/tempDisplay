#include "./tempDisplay.h"

int main(int argc, char **argv)
{
  int lcd;       // ファイルディスクリプタ:LCD1
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

//insertRecordCaptureTable(mysql,"2013-12-15 12:50:00","/var/www/tank.jpg");
//exit(1);
//ここまで----------------------------------------------------

  //スレッド処理
 printf("thread Start\n"); 
 pthread_t thread1, thread2, thread3;
  useconds_t tick1 = 200000;
  pthread_create( &thread1, NULL, thread_ledLoop, (void *) &tick1);     
  pthread_create( &thread2, NULL, thread_DBLoop, (void *) &pres);     
  pthread_create( &thread3, NULL, thread_captureLoop, (void *) &tick1);
     
  pthread_join( thread1, NULL);
  pthread_join( thread2, NULL);
  pthread_join( thread3, NULL);

  pthread_mutex_destroy(&mutex);

  return 0;
}


void *thread_captureLoop(void *ptr){

  //カメラからの画像キャプチャ
  CvCapture *capture = 0;  
  char *fileStr="/var/www/tank.jpg";
  capture=initCap();

  time_t current_time;//時間計測用構造体
  struct tm *date;
  char strDate[256];

  //スリープ時間を指定
  struct timespec ts;
  ts.tv_sec=600;//600sを指定
  ts.tv_nsec=0;//0nsを指定
 
  while(1){
    time(&current_time);
    date=localtime(&current_time);
    strftime(strDate,255,"%Y-%m-%d %H:%M:%S",date);

    saveCap(capture,fileStr);
    
    pthread_mutex_lock(&mutex);
    insertRecordCaptureTable(mysql,strDate,fileStr);
    pthread_mutex_unlock(&mutex);
    
    nanosleep(&ts,NULL);//1msスリープ

  }
}

//captureテーブルへ画像レコードを書き込みます
//datetime：日時の文字列（例"2013-1-1 1:33:22")
//filepath：画像ファイルパス
void insertRecordCaptureTable(MYSQL *con,char *datetime,char *filepath)
{

	FILE *fp = fopen(filepath, "rb");
	//ファイルが存在するかどうか
	if (fp == NULL) 
	  {
	    fprintf(stderr, "cannot open image file\n");    
	    exit(1);
	  }
	//ファイルポインタがファイル終端を示すことができるか
	fseek(fp, 0, SEEK_END);
	if (ferror(fp)) {
      
	  fprintf(stderr, "fseek() failed\n");
	  int r = fclose(fp);

	  if (r == EOF) {
	    fprintf(stderr, "cannot close file handler\n");          
	  }    
      
	  exit(1);
	}  
	
	//ファイル位置を取得できるか否か
	int flen = ftell(fp);
  
	if (flen == -1) {
      
	  perror("error occured");
	  int r = fclose(fp);

	  if (r == EOF) {
	    fprintf(stderr, "cannot close file handler\n");
	  }
      
	  exit(1);      
	}
	//ファイルポインタが先頭を示すことができるか
	fseek(fp, 0, SEEK_SET);
  
	if (ferror(fp)) {
      
	  fprintf(stderr, "fseek() failed\n");
	  int r = fclose(fp);

	  if (r == EOF) {
	    fprintf(stderr, "cannot close file handler\n");
	  }    
      
	  exit(1);
	}
	//ファイルサイズの取得
	char data[flen+1];

	int size = fread(data, 1, flen, fp);
  
	if (ferror(fp)) {
      
	  fprintf(stderr, "fread() failed\n");
	  int r = fclose(fp);

	  if (r == EOF) {
	    fprintf(stderr, "cannot close file handler\n");
	  }
      
	  exit(1);      
	}
  
	int r = fclose(fp);

	if (r == EOF) {
	  fprintf(stderr, "cannot close file handler\n");
	}

	//dateからchunkへ画像データをMysqlのバイナリに変換します
	char chunk[2*size+1];
	mysql_real_escape_string(con, chunk, data, size);

	//クエリ文字列を作成します
	char *st = "INSERT INTO capture(savetime, graphic) VALUES(cast('%1s' as datetime), '%s')";
	
	size_t st_len = strlen(st);
	char query[st_len + 2 * size+1]; 

	//クエリ文字列を結合します
	int len = snprintf(query, st_len + 2*size +1, st ,datetime, chunk);
	
	//クエリをDBへ送ります
	int errorCode;
	errorCode= mysql_real_query(con, query, len);
	switch(errorCode){
	case 0:
	printf("データ保存 ok! 返り値=%d\n",errorCode);
	break;
	case 1:
	printf("データ保存 NG! 返り値=%d\n",errorCode);
	break;
	  /*
	case CR_COMMANDS_OUT_OF_SYNC:
	  printf("CR_COMMANDS_OUT_OF_SYNC\n");
	  break;
	case CR_SERVER_GONE_ERROR:
	  printf("CR_SERVER_GONE_ERROR\n");
	  break;
	case CR_SERVER_LOST:
	  printf("CR_SERVER_LOST\n");
	  break;
	case CR_UNKNOWN_ERROR:
	  printf("CR_UNKNOWN_ERROR\m");
	  break;*/
	}

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
//LED色を決定、rgbそれぞれにOn時間を指定します
void rgbPoling(int r,int g,int b){
  //スリープ時間を指定
  struct timespec ts;
  ts.tv_sec=0;//1sを指定
  ts.tv_nsec=1000000;//0nsを指定

  int i,j;
  for(j=0;j<4;j++){
    for(i=0;i<1024;i++){
      if(i >r){
	digitalWrite(REDPIN, false); 
      }else{
	digitalWrite(REDPIN, true); 
      }
      if(i >g){
	digitalWrite(GREENPIN, false); 
      }else{
	digitalWrite(GREENPIN, true); 
      }
      if(i >b){
	digitalWrite(BLUEPIN, false); 
      }else{
	digitalWrite(BLUEPIN, true); 
      }	
    }
  }
  nanosleep(&ts,NULL);//1msスリープ

}

void *thread_ledLoop(void *ptr){
  useconds_t tick = *( int * )ptr;
  bool flagR=true,flagG=true,flagB=true;
  int counter=0,rlux=0,glux=0,blux=0;
  int currentFlag=0x01,beforeFlag=0x00,calcData=0x00;
  //スリープ時間を指定
  struct timespec ts;
  ts.tv_sec=0;//1sを指定
  ts.tv_nsec=1000000;//0nsを指定
 
  while(1){
    calcData=(currentFlag & 0x01)-(beforeFlag & 0x01);
    if(calcData<0){
      rlux--;
    }else if(calcData==0){
      ;
    }else{
      rlux++;
    }
    calcData=(currentFlag & 0x02)-(beforeFlag & 0x02);
    if(calcData<0){
      glux--;
    }else if(calcData==0){
      ;
    }else{
      glux++;
    }
    calcData=(currentFlag & 0x04)-(beforeFlag & 0x04);
    if(calcData<0){
      blux--;
    }else if(calcData==0){
      ;
    }else{
      blux++;
    }
    //printf("calcData=%d,currentData=%d,beforeData=%d,rlux=%d \r",calcData,currentFlag & 0x01,beforeFlag & 0x01,glux);
    //printf("R=%d,G=%d,B=%d \r",rlux,glux,blux);
    if(counter>1024){
      counter=0;
      beforeFlag=currentFlag;
      currentFlag++;
    }else{
      counter++;
    }
    
      //printf("%d\n",lightLux);
      //rgbPoling(0,glux,0);
      rgbPoling(rlux,glux,blux);
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
