main: main.c
	mpicc -o main main.c

run: main
	mpirun -np 16 --map-by :OVERSUBSCRIBE ./main 
