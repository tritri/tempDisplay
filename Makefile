tempDisplay.out: tempDisplay.o lib_ST7032i.o lib_mcp3002.o lib_mpl115a2.o
	gcc -o tempDisplay.out tempDisplay.o lib_ST7032i.o lib_mcp3002.o lib_mpl115a2.o -lm -I/usr/local/include:/usr/local/mysql/include -L/usr/local/lib -L/usr/local/mysql/lib -lwiringPi -lmysqlclient -g -pthread

# サフィックスルール (.c → .o)
#.c.o:
#	gcc -c $<
lib_mcp3002.o:/home/tri/source/lib/lib_mcp3002.c
	gcc -c /home/tri/source/lib/lib_mcp3002.c
lib_ST7032i.o:/home/tri/source/lib/lib_ST7032i.c
	gcc -c /home/tri/source/lib/lib_ST7032i.c
tempDisplay.o:tempDisplay.c
	gcc -c tempDisplay.c -I/usr/local/mysql/include
lib_mpl115a2.o:tempDisplay.c
	gcc -c /home/tri/source/lib/lib_mpl115a2.c

lib_mcp3002.o: /home/tri/source/lib/lib_mcp3002.h       
lib_ST7032i.o: /home/tri/source/lib/lib_ST7032i.h
test_spiDisplay.c: /home/tri/source/lib/lib_ST7032i.h
lib_mpl115a2.o: /home/tri/source/lib/lib_mpl115a2.h       
