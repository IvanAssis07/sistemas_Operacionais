#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct pagina {
   int id;
   int valida;
   int modificada;
   int referenciada;
   struct pagina *prox;
   struct pagina *pred;
} pagina;

pagina *head = NULL;
pagina *tail = NULL;
pagina *watcher = NULL;

int memoriaUtilizada = 0;
int paginaAtual = 0;
int pageFaults = 0;
int hits = 0;
int paginasEscritas = 0; 
int numeroDePaginas = 0;
unsigned tamanhoDaPagina = 0;
unsigned tamanhoDaMemoria = 0;

void fifo(struct pagina *tabelaDePaginas, int paginaAtual) {
   if (head->modificada == 1) {
      paginasEscritas++;
   }

   head->valida = 0;
   head->modificada = 0;
   head = head->prox;

   tail->prox = &tabelaDePaginas[paginaAtual];
   tail = &tabelaDePaginas[paginaAtual];
   tail->prox = NULL;
}

void lru(struct pagina *tabelaDePaginas, int paginaAtual) {
   if (head->modificada == 1) {
      paginasEscritas++;
   }

   int indice = head->id;

   tail->prox = &tabelaDePaginas[paginaAtual];
   tabelaDePaginas[paginaAtual].pred = tail;
   tabelaDePaginas[paginaAtual].prox = NULL;
   tail = &tabelaDePaginas[paginaAtual];
   head = head->prox;

   tabelaDePaginas[indice].valida = 0;
   tabelaDePaginas[indice].modificada = 0;
   tabelaDePaginas[indice].pred = NULL;
   tabelaDePaginas[indice].prox = NULL;
}

void aleatorio(struct pagina *tabelaDePaginas, int paginaAtual) {
   int numeroAleatorio = (random() % numeroDePaginas);

   pagina *aux = head;
   for (int i = 0; i < numeroAleatorio; i++) {
      aux = aux->prox;
   };

   int indice = aux->id;

   if (aux->modificada == 1) {
      paginasEscritas++;
   }

   if( (aux->id != head->id) && (aux->id != tail->id)){
      tabelaDePaginas[paginaAtual].prox = aux->prox;
      tabelaDePaginas[paginaAtual].pred = aux->pred;
      aux->pred->prox = &tabelaDePaginas[paginaAtual];
      aux->prox->pred = &tabelaDePaginas[paginaAtual];
   }

   if (aux->id == head->id) {
      tabelaDePaginas[paginaAtual].prox = head->prox;
      tabelaDePaginas[paginaAtual].pred = NULL;
      head->prox->pred = &tabelaDePaginas[paginaAtual];
      head = &tabelaDePaginas[paginaAtual];
   }

   if (aux->id == tail->id) {
      tabelaDePaginas[paginaAtual].prox = NULL;
      tabelaDePaginas[paginaAtual].pred = tail->pred;
      tail->pred->prox = &tabelaDePaginas[paginaAtual];
      tail = &tabelaDePaginas[paginaAtual];
   }

   aux->valida = 0;
   aux->modificada =0;
   aux->pred = NULL;
   aux->prox = NULL;
}

void segundaChance(struct pagina *tabelaDePaginas, int paginaAtual) {
   pagina *aux = watcher;
   while (aux->referenciada != 0) {
      aux->referenciada = 0;
      aux = aux->prox;
   }
   watcher = aux->prox;

   if (aux->modificada == 1) {
      paginasEscritas++;
   }

   aux->pred->prox = &tabelaDePaginas[paginaAtual];
   aux->prox->pred = &tabelaDePaginas[paginaAtual];
   tabelaDePaginas[paginaAtual].prox = aux->prox;
   tabelaDePaginas[paginaAtual].pred = aux->pred;

   aux->valida = 0;
   aux->modificada = 0;
   aux->referenciada = 0;
   aux->pred = NULL;
   aux->prox = NULL;
}

void substituirPagina(char algoritmoDeSubstituicao[100],struct pagina *tabelaDePaginas, int paginaAtual, char tipoAcesso){
   if (strcmp(algoritmoDeSubstituicao, "lru") == 0) {
      lru(tabelaDePaginas, paginaAtual);
   }
   else if (strcmp(algoritmoDeSubstituicao, "2a") == 0) {
      segundaChance(tabelaDePaginas, paginaAtual);
   }
   else if (strcmp(algoritmoDeSubstituicao, "fifo") == 0) {
      fifo(tabelaDePaginas, paginaAtual);
   }
   else if (strcmp(algoritmoDeSubstituicao, "random") == 0) {
      aleatorio(tabelaDePaginas, paginaAtual);
   }
}

void atualizarPilhaLru(struct pagina *tabelaDePaginas, int paginaAtual) {
   if ((tabelaDePaginas[paginaAtual].id != head->id) && (tabelaDePaginas[paginaAtual].id != tail->id)) {
      pagina *auxProximo = tabelaDePaginas[paginaAtual].prox;
      pagina *auxPredecessor = tabelaDePaginas[paginaAtual].pred;

      auxPredecessor->prox = auxProximo;
      auxProximo->pred = auxPredecessor;

      tail->prox = &tabelaDePaginas[paginaAtual];
      tabelaDePaginas[paginaAtual].pred = tail;
      tabelaDePaginas[paginaAtual].prox = NULL;
      tail = &tabelaDePaginas[paginaAtual];
   }
   
   if (tabelaDePaginas[paginaAtual].id == head->id) {
      head->prox->pred = NULL;
      head = head->prox;

      tail->prox = &tabelaDePaginas[paginaAtual];
      tabelaDePaginas[paginaAtual].pred = tail;
      tabelaDePaginas[paginaAtual].prox = NULL;
      tail = &tabelaDePaginas[paginaAtual];
   } 
            
}

int main(int argc, char *argv[]) {
   // Comentar quando for executar os testes
   printf("Executando o simulador...\n");
   srandom(42);
   char algoritmoDeSubstituicao[100];
   char arquivoDeEntrada[100];
   tamanhoDaPagina = atoi(argv[3]);
   tamanhoDaMemoria = atoi(argv[4]);
   numeroDePaginas = tamanhoDaMemoria/tamanhoDaPagina;

   struct pagina *tabelaDePaginas = malloc(2097152 * sizeof(struct pagina));

   strcpy(algoritmoDeSubstituicao, argv[1]);
   strcpy(arquivoDeEntrada, argv[2]);

   unsigned s, tmp;
   /* Derivar o valor de s: */
   tmp = tamanhoDaPagina * 1024;
   s = 0;
   while (tmp>1) {
      tmp = tmp>>1;
      s++;
   }

   FILE * fp;
   unsigned addr;
   char rw;
   
   fp = fopen(arquivoDeEntrada, "r");
   if (fp == NULL) {
      exit(EXIT_FAILURE);
   }

   while (fscanf(fp, "%x %c", &addr, &rw) == 2) {
      paginaAtual = addr >> s;
      if (rw == 'W') {
         tabelaDePaginas[paginaAtual].modificada = 1;
      }
      if (tabelaDePaginas[paginaAtual].valida == 1) {
         if ((strcmp(algoritmoDeSubstituicao, "lru") == 0)) {
            hits++;
            if (tabelaDePaginas[paginaAtual].id != tail->id) {
               atualizarPilhaLru(tabelaDePaginas, paginaAtual);
            }
         } else {
            hits++;
            tabelaDePaginas[paginaAtual].referenciada = 1;
         }
      } else {
         pageFaults++;
         tabelaDePaginas[paginaAtual].id = paginaAtual;
         tabelaDePaginas[paginaAtual].valida = 1;
         tabelaDePaginas[paginaAtual].referenciada = 0;

         if (memoriaUtilizada == 0) {
            if ((strcmp(algoritmoDeSubstituicao, "2a") == 0)) {
               tabelaDePaginas[paginaAtual].pred = &tabelaDePaginas[paginaAtual];
               tabelaDePaginas[paginaAtual].prox = &tabelaDePaginas[paginaAtual];
               head = &tabelaDePaginas[paginaAtual];
               tail = &tabelaDePaginas[paginaAtual];
               watcher = head;
               memoriaUtilizada++;
            } else {
               tabelaDePaginas[paginaAtual].pred = NULL;
               tabelaDePaginas[paginaAtual].prox = NULL;
               head = &tabelaDePaginas[paginaAtual];
               tail = &tabelaDePaginas[paginaAtual];
               memoriaUtilizada++;
            }
         }
         else if (memoriaUtilizada < numeroDePaginas) {
            tabelaDePaginas[paginaAtual].pred = tail;
            tail->prox = &tabelaDePaginas[paginaAtual];
            tail = &tabelaDePaginas[paginaAtual];

            if (strcmp(algoritmoDeSubstituicao, "2a") == 0) {
               head->pred = tail;
               tail->prox = head;
            } else {
               tail->prox = NULL;
            }
            
            memoriaUtilizada++;
         } else {
            substituirPagina(algoritmoDeSubstituicao, tabelaDePaginas, paginaAtual, rw);
         }
      }
   }

   fclose(fp);

  //  printf("Arquivo de entrada: %s\n", arquivoDeEntrada);
  //  printf("Tamanho da memoria: %d\n", tamanhoDaMemoria);
  //  printf("Tamanho da pagina: %d\n", tamanhoDaPagina);
  //  printf("Algoritmo de substituicao: %s\n", algoritmoDeSubstituicao);
  //  printf("paginas lidas: %d\n", pageFaults);
  //  printf("paginas escritas: %d\n", paginasEscritas);

   // Usar para gerar dados de testes junto com script, comentar o prints e o Executando acima
  printf("%s,%d,%d,%d,%d,%d,%d", algoritmoDeSubstituicao, tamanhoDaMemoria, tamanhoDaPagina, numeroDePaginas, pageFaults, paginasEscritas,  hits);

   return 0;
}