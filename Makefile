CXX := gcc
CC:=gcc
LDFLAGS := -g -pthread -L/usr/lib -L/usr/local/lib -L/usr/local/mysql/lib
LIBS:=-lm -lopencv_legacy -lopencv_highgui -lopencv_core -lopencv_ml -lopencv_video -lopencv_imgproc -lopencv_calib3d -lopencv_objdetect -lwiringPi -lmysqlclient 

#CPPFLAGS = -g -I/usr/include/opencv -I/usr/include/opencv2 -g -pthread
CFLAGS:=-O4 -Wall -I/usr/local/include -I/usr/local/mysql/include -I/usr/include/opencv -I/usr/include/opencv2
PROGRAM:=tempDisplay.out
OBJS:=tempDisplay.o lib_ST7032i.o lib_mcp3002.o lib_mpl115a2.o lib_capture.o
#all:$(PROGARM)

$(PROGRAM):$(OBJS)
	$(CC) -o $(PROGRAM) $(OBJS) $(CFLAGS) $(LDFLAGS) $(LIBS)   

#tempDisplay.out: tempDisplay.o lib_ST7032i.o lib_mcp3002.o lib_mpl115a2.o lib_capture.o
#	gcc -o tempDisplay.out tempDisplay.o lib_ST7032i.o lib_mcp3002.o lib_mpl115a2.o lib_capture.o -lm -I/usr/local/include:/usr/local/mysql/include -L/usr/local/lib -L/usr/local/mysql/lib -lwiringPi -lmysqlclient -g -pthread

# サフィックスルール (.c → .o)
#.c.o:
#	gcc -c $<
tempDisplay.o: ./tempDisplay.c
	$(CC) $(CFLAGS) -c tempDisplay.c
lib_mcp3002.o: ../lib/lib_mcp3002.c
	$(CC) -c ../lib/lib_mcp3002.c
lib_ST7032i.o: ../lib/lib_ST7032i.c
	$(CC) -c ../lib/lib_ST7032i.c
lib_mpl115a2.o: ../lib/lib_mpl115a2.c
	$(CC) -c ../lib/lib_mpl115a2.c
lib_capture.o: ../lib/lib_capture.c
	$(CC) -c ../lib/lib_capture.c

lib_capture.o: ../lib/lib_capture.h       
lib_mcp3002.o: ../lib/lib_mcp3002.h       
lib_ST7032i.o: ../lib/lib_ST7032i.h
#test_spiDisplay.c: /home/tri/source/lib/lib_ST7032i.h
lib_mpl115a2.o: ../lib/lib_mpl115a2.h       
tempDisplay.o:	./tempDisplay.h