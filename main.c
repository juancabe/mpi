#include <assert.h>
#include <limits.h>
#include <mpi.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#define ITERACIONES_SIN_MIRAR 200000

#define NUM_MAX 100000000
#define TAMANHO_LISTA 80
#define SETS_LISTA 3

#define SETS 3
#define TAMANHO 50

// Definición de tags para la comunicación
#define TAG_FOUND 100
#define TAG_STOP 101

unsigned long EST_SEND_STOP = 0;
unsigned long EST_SEND_FIND = 0;
unsigned long EST_RECV_STOP = 0;
unsigned long EST_RECV_FIND = 0;
unsigned long EST_BCAST_NUMERO = 0;
unsigned long EST_BCAST_FIN = 0;
unsigned long EST_REDUCE_TPO = 0;
unsigned long EST_REDUCE_INTENTOS = 0;
unsigned long EST_REDUCE_EST = 0;
unsigned long EST_IPROBE_FIND = 0;
unsigned long EST_IPROBE_STOP = 0;

int pID = 0;
MPI_Status status;

unsigned int numeros[SETS_LISTA][TAMANHO_LISTA] = {{8, 66, 748, 1158, 10231, 497134, 4608707, 47820120, 2, 41, 578, 1423, 52611, 770895, 2790721, 34643109, 8, 10, 851, 2209, 69990, 159627, 3385804, 15821867, 5, 13, 692, 1378, 20023, 820983, 5989397, 71761937, 6, 62, 127, 8986, 36126, 971985, 5739524, 70900793, 3, 43, 692, 6066, 60555, 844783, 3509650, 93991716, 1, 56, 859, 2407, 41146, 288994, 9565675, 70779739, 9, 47, 876, 2456, 20591, 920964, 4403757, 68563587, 2, 21, 355, 7035, 58257, 956057, 2244822, 98602552, 2, 90, 306, 2971, 95222, 814239, 7627087, 54434251},
                                                   {4, 77, 886, 7564, 50085, 993399, 4874938, 74030116, 9, 23, 737, 3708, 67607, 956008, 6248183, 43669844, 7, 50, 684, 5893, 69898, 200014, 2136757, 91838319, 8, 75, 877, 2507, 65557, 559730, 8465332, 75839092, 1, 94, 144, 2057, 61882, 864629, 8394357, 80773957, 9, 74, 224, 1417, 21923, 362955, 8884601, 49938426, 9, 15, 573, 2464, 66176, 422314, 9860085, 88384300, 8, 55, 153, 2757, 54958, 912936, 5326961, 30264300, 5, 69, 299, 6618, 10463, 556147, 8711726, 95397660, 3, 29, 684, 5303, 58626, 364864, 8198828, 80756457},
                                                   {1, 44, 763, 6169, 67524, 919717, 1473763, 13587296, 5, 71, 569, 1163, 11550, 281387, 8255291, 93701450, 5, 52, 302, 4630, 61202, 218275, 7011910, 75292541, 1, 91, 505, 2139, 65462, 978520, 9179619, 24758500, 9, 41, 901, 5923, 31806, 660136, 6392446, 16262444, 8, 40, 543, 2759, 57699, 282161, 6890176, 39677629, 8, 65, 247, 6476, 88372, 558802, 7420916, 28514707, 5, 62, 103, 2025, 29201, 719487, 6806547, 97055905, 1, 11, 717, 6131, 75383, 936184, 4064483, 14741509, 5, 87, 136, 3076, 60375, 528071, 8505013, 67316720}};

double mygettime(void) {
  struct timeval tv;
  if (gettimeofday(&tv, 0) < 0) {
    perror("oops");
  }
  return (double)tv.tv_sec + (0.000001 * (double)tv.tv_usec);
}

static unsigned int next = 1;

#define MY_RAND_MAX 2147483647

int myrand(void) {
  next = next * 1103515245 + 12345;
  return ((unsigned)(next % MY_RAND_MAX));
}

void mysrand(unsigned int seed) { next = seed; }

// En el proceso padre devuelve el hijo que lo ha encontrado.
int busca_numero(unsigned int numero, unsigned long long *intentos, double *tpo) {
  short bSalir = 0;
  double t1, t2;
  *intentos = 0;
  t1 = mygettime();
  int i = 0;
  int flag = 0;
  int parar = 0;
  int para;
  int encontrado = 0; // Proceso que encuentra el número, por defecto es el padre

  do {
    (*intentos)++;

    if ((myrand() % (NUM_MAX - 1) + 1) == numero) { // Numero encontrado
      bSalir = 1;
      if (pID != 0) { // Encontrado, por defecto, es 0, si el padre lo encuentra no tiene que hacer nada
        EST_SEND_FIND++;
        MPI_Send(&pID, 1, MPI_INT, 0, TAG_FOUND, MPI_COMM_WORLD); // Decirle al padre que lo hemos encontrado
        EST_RECV_STOP++;
        MPI_Recv(&para, 1, MPI_INT, 0, TAG_STOP, MPI_COMM_WORLD, &status); // Esperar a que nos confirme la recepción, nos sincroniza con los que no han encontrado el número
      }
    }
    if (pID == 0) {                     // Comprobar si alguien lo ha encontrado
      if (i == ITERACIONES_SIN_MIRAR) { // Cada ITERACIONES_SIN_MIRAR iteraciones
        EST_IPROBE_FIND++;
        MPI_Iprobe(MPI_ANY_SOURCE, TAG_FOUND, MPI_COMM_WORLD, &flag, &status); // Recibir TAG_FOUND primer proceso en encontrar
        if (flag) {
          int mensajero;
          EST_RECV_FIND++;
          MPI_Recv(&mensajero, 1, MPI_INT, status.MPI_SOURCE, TAG_FOUND, MPI_COMM_WORLD, &status);
          encontrado = mensajero; // Lo ha encontrado "mensajero"
          bSalir = 1;
        }
        i = 0;
      }
      i++;
    } else { // Comprobar si hay que parar
      if (i == ITERACIONES_SIN_MIRAR) {
        EST_IPROBE_STOP++;
        MPI_Iprobe(0, TAG_STOP, MPI_COMM_WORLD, &parar, &status);
        if (parar) {
          // Consumir el mensaje de parada
          EST_RECV_STOP++;
          MPI_Recv(&para, 1, MPI_INT, 0, TAG_STOP, MPI_COMM_WORLD, &status);
          bSalir = 1;
        }
        i = 0;
      }
      i++;
    }
  } while (!bSalir);
  t2 = mygettime();
  *tpo = t2 - t1;
  return encontrado;
}

int main(int argc, char *argv[]) {

  MPI_Init(&argc, &argv);

  int iNP, numero;
  unsigned long long intentos_totales = 0;
  double tpo_total = 0.0;
  int a;

  MPI_Comm_size(MPI_COMM_WORLD, &iNP); // Devuelve el número de procesos "invocados"
  MPI_Comm_rank(MPI_COMM_WORLD, &pID); // Devuelve el PID de MPI

  if (pID == 0) {
    printf("RAND_MAX : %d\n", MY_RAND_MAX);
    printf("NUM_MAX : %d\n", NUM_MAX);
    printf("UINT_MAX : %u\n", UINT_MAX);
    printf("ULLONG_MAX: %llu\n", ULLONG_MAX);
  }

  unsigned long encontrados_por_proceso[iNP]; // Array para guardar el número de aciertos por proceso

  // -- Arrays para acumular estadísticas de los sets. --
  unsigned long long intentos_acumulados_set[SETS];
  double tiempo_acumulado_set[SETS];
  double tiempo_total_prueba_set[SETS];
  // --                                                --
  memset(encontrados_por_proceso, 0, sizeof(unsigned long) * iNP);

  mysrand(pID);

  double tiempo_inicio_total = mygettime();

  for (int i = 0; i < SETS_LISTA; i++) {
    if (pID == 0)
      printf("*****************************************************************************\n");
      printf("SET: %d\n", i);
    for (int j = 0; j < TAMANHO; j++) {
      if (pID == 0)
        numero = numeros[i][j];

      EST_BCAST_NUMERO++;
      MPI_Bcast(&numero, 1, MPI_INT, 0, MPI_COMM_WORLD); // PADRE: Comunicar a todos el numero a adivinar
                                                         // HIJOS: Reciben el número a adivinar

      if (pID != 0) {
        j = 0; // Los hijos se quedan en el bucle for interior infinitamente
      }

      if (numero == -1) {
        break;
      }

      unsigned long long intentos = 0;
      double tpo = 0.0;
      int encontrado = busca_numero(numero, &intentos, &tpo);

      if (pID == 0) {
        int para = 0; // "para" es un buffer que no significa nada, no queremos enviar info, solo indicar que paren
        for (int k = 1; k < iNP; k++) {
          EST_SEND_STOP++;
          MPI_Send(&para, 1, MPI_INT, k, TAG_STOP, MPI_COMM_WORLD);
        }
      }

      unsigned long long sumar_intentos = 0; // Intentos acumulados de todos los procesos
      double sumar_tpo = 0.0;                // Tiempo máximo en busca_numero para todos los procesos

      // -- Conseguir datos de todos los procesos, sincronizándolos --
      EST_REDUCE_INTENTOS++;
      MPI_Reduce(&intentos, &sumar_intentos, 1, MPI_UNSIGNED_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
      EST_REDUCE_TPO++;
      MPI_Reduce(&tpo, &sumar_tpo, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
      // --                                                         --
      intentos_acumulados_set[i] += sumar_intentos;
      tiempo_acumulado_set[i] += sumar_tpo;
      tiempo_total_prueba_set[i] += tpo;

      if (pID == 0) { // Imprimir datos de esta ronda de busca_numero

        intentos_totales += sumar_intentos;
        tpo_total += sumar_tpo; // HECHO: Sería += sumar_tpo
        printf("S:%d/%d, N:%2d/%d) Num:%10d, Int:%10llu, Tpo: %.8f (ENCONTRADO POR %d)", i + 1, SETS_LISTA, j + 1, TAMANHO, numero, sumar_intentos, tpo, encontrado);

        encontrados_por_proceso[encontrado]++;

        int flag;
        int mensajero;
        a = 0;
        while (++EST_IPROBE_FIND && !MPI_Iprobe(MPI_ANY_SOURCE, TAG_FOUND, MPI_COMM_WORLD, &flag, &status) && flag) {
          // Miramos si alguno más lo encontró
          EST_RECV_FIND++;
          MPI_Recv(&mensajero, 1, MPI_INT, status.MPI_SOURCE, TAG_FOUND, MPI_COMM_WORLD, &status);
          if (a == 0) {
            printf(" |También encontrado por %d", mensajero); // Había un error
          } else {
            printf(", %d", encontrado);
          }
          a++;
        }
        printf("\n");
      }
    }

    if (pID == 0) {
      printf("Resumen SET %d:\nIntentos Acumulados: %llu\nTiempo Acumulado: %.8f\nTiempo Total Prueba: %.6f\nIntentos por segundo: %.6f\n", i, intentos_acumulados_set[i], tiempo_acumulado_set[i], tiempo_total_prueba_set[i], intentos_acumulados_set[i] / tiempo_total_prueba_set[i]);
    } else if (numero == -1) {
      // Los hijos se salen si el número es -1
      break;
    } else {
      // Parte del código que no se debería alcanzar (los hijos no deben salir del bucle interior con un numero != 1)
      assert(false);
    }
  }

  if (pID == 0) {
    int numero = -1;
    EST_BCAST_FIN++;
    MPI_Bcast(&numero, 1, MPI_INT, 0, MPI_COMM_WORLD);
  }

  MPI_Barrier(MPI_COMM_WORLD);

  unsigned long recv_buffer;

  EST_REDUCE_EST++;
  MPI_Reduce(&EST_SEND_STOP, &recv_buffer, 1, MPI_UNSIGNED_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
  EST_SEND_STOP = recv_buffer;

  EST_REDUCE_EST++;
  MPI_Reduce(&EST_SEND_FIND, &recv_buffer, 1, MPI_UNSIGNED_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
  EST_SEND_FIND = recv_buffer;

  EST_REDUCE_EST++;
  MPI_Reduce(&EST_RECV_STOP, &recv_buffer, 1, MPI_UNSIGNED_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
  EST_RECV_STOP = recv_buffer;

  EST_REDUCE_EST++;
  MPI_Reduce(&EST_RECV_FIND, &recv_buffer, 1, MPI_UNSIGNED_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
  EST_RECV_FIND = recv_buffer;

  EST_REDUCE_EST++;
  MPI_Reduce(&EST_BCAST_NUMERO, &recv_buffer, 1, MPI_UNSIGNED_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
  EST_BCAST_NUMERO = recv_buffer;

  EST_REDUCE_EST++;
  MPI_Reduce(&EST_BCAST_FIN, &recv_buffer, 1, MPI_UNSIGNED_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
  EST_BCAST_FIN = recv_buffer;

  EST_REDUCE_EST++;
  MPI_Reduce(&EST_REDUCE_TPO, &recv_buffer, 1, MPI_UNSIGNED_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
  EST_REDUCE_TPO = recv_buffer;

  EST_REDUCE_EST++;
  MPI_Reduce(&EST_REDUCE_INTENTOS, &recv_buffer, 1, MPI_UNSIGNED_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
  EST_REDUCE_INTENTOS = recv_buffer;

  EST_REDUCE_EST++;
  MPI_Reduce(&EST_IPROBE_FIND, &recv_buffer, 1, MPI_UNSIGNED_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
  EST_IPROBE_FIND = recv_buffer;

  EST_REDUCE_EST++;
  MPI_Reduce(&EST_IPROBE_STOP, &recv_buffer, 1, MPI_UNSIGNED_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
  EST_IPROBE_STOP = recv_buffer;

  EST_REDUCE_EST++;
  MPI_Reduce(&EST_REDUCE_EST, &recv_buffer, 1, MPI_UNSIGNED_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
  EST_REDUCE_EST = recv_buffer;

  if (pID == 0) {

    double tiempo_fin_total = mygettime();
    double tiempo_total_prueba = tiempo_fin_total - tiempo_inicio_total;

    printf("\n\n============================================================================\n");
    printf("FINAL:\n");
    printf("============================================================================\n");
    printf("Intentos Acumulados : %llu\n", intentos_totales);
    printf("Tiempo Acumulado Procesos: %.6f\n", tpo_total);
    printf("Tiempo Total Prueba : %.6f\n", tiempo_total_prueba);
    printf("Intentos por segundo : %.6f\n", (double)intentos_totales / tiempo_total_prueba);
    printf("============================================================================\n");
    printf("LLAMADAS MPI:\n");
    printf("============================================================================\n");
    printf("EST_SEND_STOP : %lu\n", EST_SEND_STOP);
    printf("EST_SEND_FIND : %lu\n", EST_SEND_FIND);
    printf("EST_RECV_STOP : %lu\n", EST_RECV_STOP);
    printf("EST_RECV_FIND : %lu\n", EST_RECV_FIND);
    printf("EST_BCAST_NUMERO : %lu\n", EST_BCAST_NUMERO);
    printf("EST_BCAST_FIN : %lu\n", EST_BCAST_FIN);
    printf("EST_REDUCE_TPO : %lu\n", EST_REDUCE_TPO);
    printf("EST_REDUCE_INTENTOS: %lu\n", EST_REDUCE_INTENTOS);
    printf("EST_REDUCE_EST : %lu\n", EST_REDUCE_EST);
    printf("EST_IPROBE_FIND : %lu\n", EST_IPROBE_FIND);
    printf("EST_IPROBE_STOP : %lu\n", EST_IPROBE_STOP);
    printf("============================================================================\n");
    printf("ENCONTRADOS:\n");

    for (int i = 0; i < iNP; i++) {
      printf("%d: %lu (%.6f)\n", i, encontrados_por_proceso[i], encontrados_por_proceso[i] * (100.0 / (SETS_LISTA * TAMANHO)));
    }
  }

  MPI_Finalize();
  return 0;
}
