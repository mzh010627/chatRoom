TARGET = client
OBJ = main.o Client.o

$(TARGET):$(OBJ)
	gcc $^ -o $@ -ljson-c


main.o:main.c
	gcc -c $^ -o $@ 

Client.o:Client.c
	gcc -c $^ -o $@

clean:
	@rm -rf *.o $(TARGET)