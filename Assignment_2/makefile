
IN = 28
K = 2
TESTDIR = ./materials/Test_Cases/large_files

s:
	cd Server; gcc -o server server.c ; ./server

c:
	cd Client; gcc -o client client.c ; echo "example.txt\n${K}\n" | ./client

clean:
	rm ./Server/*.txt ./Server/*.enc
	rm ./Client/*.enc

check:
	cp ${TESTDIR}/InputFiles/*${IN}* ./Client/example.txt
	touch ./Client/example.txt.enc
	rm ./Client/*.enc
	make c
	diff ./Client/example.txt.enc ${TESTDIR}/*${K}*/*${IN}*
	@make clean
