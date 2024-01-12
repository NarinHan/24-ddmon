comp:
	gcc -g -o target target.c -pthread
	gcc -shared -fPIC -o ddmon.so ddmon.c -pthread -ldl
	gcc -o ddchck ddchck.c -pthread

clean:
	rm target ddmon.so ddchck
