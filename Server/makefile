#自定义变量
OBJ = $(patsubst %.c, %.o, $(wildcard ./*.c))
TARGET = a.out

#使用变量：$(变量名)
$(TARGET): $(OBJ)
	gcc $^ -o $@ -ljson-c -lmysqlclient


clean:
	@rm -rf *.o $(TARGET)