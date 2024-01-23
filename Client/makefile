#自定义变量
OBJ = main.o ChatRoom.o
TARGET = a.out

#使用变量：$(变量名)
$(TARGET): $(OBJ)
	gcc $^ -o $@ -ljson-c 

ChatRoom.o: ChatRoom.c
	gcc -c $^ -o $@

clean:
	@rm -rf *.o $(TARGET)