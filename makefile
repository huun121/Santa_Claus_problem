ARGS=-std=gnu99 -Wall -Wextra -Werror -pedantic -pthread -lrt

proj2:
	gcc proj2.c $(ARGS) -o proj2

clean:
	rm proj2