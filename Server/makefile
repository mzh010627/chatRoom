src=*.c
server: ${src}
	gcc ${src} -o server -ljson-c -lmysqlclient

g:${src}
	gcc -g ${src} -o gtest

clean:
	@rm -rf server gtest