/*
 * counter-proc.c
 *
 *  Created on: 16 mag 2020
 *      Author: utente
 *
 *      PRESO DA REPL
 *      esercizio "esempio di sincronizzazione sbagliata: https://repl.it/@MarcoTessarotto/counter2proc"
 *		risolvo il problema utilizzando un semaforo senza nome!
 *		Occorre  allora creare una memory mapping dove mettere il semaforo!
 *
 */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>        /* For mode constants */
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>
#include <semaphore.h>

sem_t * process_semaphore;

#define CHECK_ERR(a,msg) {if ((a) == -1) { perror((msg)); exit(EXIT_FAILURE); } }

#define CHECK_ERR_MMAP(a,msg) {if ((a) == MAP_FAILED) { perror((msg)); exit(EXIT_FAILURE); } }



sem_t * create_anon_mmap(size_t size) {

	// MAP_SHARED: condivisibile tra processi
	// PROT_READ | PROT_WRITE: posso leggere e scrivere nello spazio di memoria
	process_semaphore = mmap(NULL, size,
			PROT_READ | PROT_WRITE,
			MAP_SHARED | MAP_ANONYMOUS,
			-1,
			0);

	CHECK_ERR_MMAP(process_semaphore,"mmap")
	/*
	if (memory == MAP_FAILED) {
		perror("mmap");

		return NULL;
	}
	*/
  // l'indirizzo restituito da mmap è "allineato a pagina" (page aligned)
  if (((unsigned long)process_semaphore & 4095) == 0) {
     printf("memory is page aligned\n");
  }

	return process_semaphore;

}


int * shared_counter;

#define CYCLES 1000000

int main(void) {

  int res;

  process_semaphore = create_anon_mmap(sizeof(sem_t)+sizeof(int));

  shared_counter = (int *) (process_semaphore+1);

  printf("shared_counter initial value: %d\n", *shared_counter);


  res = sem_init(process_semaphore,
			1, // 1 => il semaforo è condiviso tra processi, 0 => il semaforo è condiviso tra threads del processo
			1 // valore iniziale del semaforo
		  );

  CHECK_ERR(res,"sem_init")



  if ((res = fork()) == -1) {
    perror("fork()\n");
    exit(EXIT_FAILURE);
  } else if ( res == 0) {

  printf("pid=%d\n", res);

	  for (int i = 0; i < CYCLES; i++) {

		// questa è la sezione critica perché i 2 processi ci accedono CONTEMPORANEAMENTE
		// occorre mettere un semaforo qui!
			if (sem_wait(process_semaphore) == -1) {
				perror("sem_wait");
				exit(EXIT_FAILURE);
			}


			(*shared_counter)++; // <==== problema di sincronizzazione
		// vedere pag. 5 del Little Book of Semaphores
		// http://greenteapress.com/semaphores/LittleBookOfSemaphores.pdf
			if (sem_post(process_semaphore) == -1) {
					perror("sem_post");
					exit(EXIT_FAILURE);
				}


	  }
	  exit(EXIT_SUCCESS); // fine del processo figlio
  } else {
	  printf("pid=%d\n", res);

	  	  for (int i = 0; i < CYCLES; i++) {

	  		// questa è la sezione critica perché i 2 processi ci accedono CONTEMPORANEAMENTE
	  		// occorre mettere un semaforo qui!
	  			if (sem_wait(process_semaphore) == -1) {
	  				perror("sem_wait");
	  				exit(EXIT_FAILURE);
	  			}


	  			(*shared_counter)++; // <==== problema di sincronizzazione
	  		// vedere pag. 5 del Little Book of Semaphores
	  		// http://greenteapress.com/semaphores/LittleBookOfSemaphores.pdf
	  			if (sem_post(process_semaphore) == -1) {
	  					perror("sem_post");
	  					exit(EXIT_FAILURE);
	  				}


	  	  }
  }

  wait(NULL);

  // il risultato che ci aspetteremmo è CYCLES * 2, invece....
  printf("expected result=%d, counter value=%d\n", CYCLES * 2, *shared_counter);

  // c'è un caso in cui il risultato è CYCLES * 2, quale è?
  // **** PROVARE AD IMPLEMENTARE UN SEMAFORO ***
  // il semaforo senza nome va distrutto solo quando non ci sono processi bloccati su di esso
  res = sem_destroy(process_semaphore);

  CHECK_ERR(res,"sem_destroy")

  printf("[parent] bye %u\n",getpid());


  return 0;
}
