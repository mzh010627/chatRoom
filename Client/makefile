TARGET = client
OBJ = main.o ChatRoom.o

$(TARGET):$(OBJ)
	gcc $^ -o $@ -ljson-c

g:*c
	gcc -g $^ -o $@ -ljson-c
main.o:main.c
	gcc -c $^ -o $@ 

ChatRoom.o:ChatRoom.c
	gcc -c $^ -o $@

clean:
	@rm -rf *.o $(TARGET)