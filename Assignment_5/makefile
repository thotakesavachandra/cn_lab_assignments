init: libmsocket.a initmsocket.c
	gcc initmsocket.c -o init -L. -lmsocket;

run: init
	./init;

libmsocket.a: msocket.o
	ar rcs libmsocket.a msocket.o

msocket.o: msocket.c msocket.h
	gcc -c msocket.c -o msocket.o;

clean:
	rm -f msocket.o libmsocket.a u1 u2 init;
	rm -f user1 user2;
	rm -f u0 u1 u2 u3 u1.c u2.c u3.c;
	cd output_files; rm u0_recv.txt u1_recv.txt u2_recv.txt u3_recv.txt;



############################################# 2 USERS #############################################

user1: libmsocket.a
	gcc user1.c -o user1 -L. -lmsocket;

runuser1: user1
	./user1;

user2: libmsocket.a
	gcc user2.c -o user2 -L. -lmsocket;

runuser2: user2
	./user2;



############################################# 4 USERS #############################################

u0: u0.c
	gcc u0.c -o u0 -L. -lmsocket -DMYID=0;

u1: u0.c
	gcc u0.c -o u1 -L. -lmsocket -DMYID=1;

u2 : u0.c
	cp u0.c u2.c;
	gcc u0.c -o u2 -L. -lmsocket -DMYID=2;

u3: u0.c
	cp u0.c u3.c;
	gcc u0.c -o u3 -L. -lmsocket -DMYID=3;

runu0: u0
	touch ./output_files/u0_recv.txt;
	./u0;	

runu1: u1
	touch ./output_files/u1_recv.txt;
	./u1;

runu2: u2
	touch ./output_files/u2_recv.txt;
	./u2;

runu3: u3
	touch ./output_files/u3_recv.txt;
	./u3;


