#自定义变量
OBJ = demoServer.o
TARGET = a.out

#使用变量：$(变量名)
$(TARGET): $(OBJ)
	gcc $^ -o $@ -ljson-c -lmysqlclient


clean:
	@rm -rf *.o $(TARGET)