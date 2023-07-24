#include <ucontext.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <string.h>
#include <malloc.h>

#include "dccthread.h"
#include "dlist.h"

typedef struct dccthread{
    char name[DCCTHREAD_MAX_NAME_SIZE];
    ucontext_t context;
    dccthread_t* waitingThread; /* Indica qual thread esta deve esperar terminar 
                                 * para ser a vez dela. Se for NULL não há o que esperar*/
}dccthread_t;

//thread main
dccthread_t* mainThread;

//contexto manager
dccthread_t* manager;

// Lista de threads prontas
struct dlist* readyThreadsList;
struct dlist* sleepingThreadsList;

// A pointer for the curruent thread
dccthread_t *currentThread;

// Váriáveis do timer
timer_t timer;
struct sigevent sigEvent;
struct sigaction sigAction;
struct itimerspec sigIts;

// Sleep Timer
timer_t sleepTimer;
struct sigevent sleepEvent;
struct sigaction sleepAction;
struct itimerspec sleepIts;

// Variáveis condição de corrida
sigset_t timerMask; 
sigset_t sleepMask;

int existThread(dccthread_t *target){
	if(dlist_empty(readyThreadsList) && dlist_empty(sleepingThreadsList)) return 0;
	
	struct dnode *currThread = readyThreadsList->head;
	while(currThread != NULL && (dccthread_t *)currThread->data != target) currThread = currThread->next;
	
	if(currThread != NULL) return 1;
	
	currThread = sleepingThreadsList->head;
	while(currThread != NULL && (dccthread_t *)currThread->data != target) currThread = currThread->next;
	
	return currThread != NULL;
}

void timerHandler(int signum){
    dccthread_yield();
}

// Timer para realizar a preempção
void startTimer(){
    // Setando o que fazer com o sinal gerado quando o tempo terminar
    sigAction.sa_handler = timerHandler;
    sigaction(SIGRTMIN, &sigAction, NULL);

    // Infos sobre o sinal que deve ser enviado
    sigEvent.sigev_notify = SIGEV_SIGNAL;
    sigEvent.sigev_signo = SIGRTMIN; // Sinal numérico a ser enviado
    sigEvent.sigev_value.sival_ptr = &timer;
    timer_create(CLOCK_PROCESS_CPUTIME_ID, &sigEvent, &timer);

    // Definindo o timer
    sigIts.it_value.tv_nsec = 10000000; // Tempo inicial para expirar
    sigIts.it_interval.tv_nsec = 10000000; // Tempo para expirações subsequentes

    timer_settime(timer, 0, &sigIts, NULL);
}

void managerFunction(int param) {

    while (!dlist_empty(readyThreadsList) || !dlist_empty(sleepingThreadsList)){

        // Desbloqueia o sinal brevemente para possibilitar threads dormindo acordarem
        sigprocmask(SIG_UNBLOCK, &sleepMask, NULL);
        sigprocmask(SIG_BLOCK, &sleepMask, NULL);

        // Troca o contexto do manager para prox thread
        currentThread = (dccthread_t *) dlist_pop_left(readyThreadsList);

        // Testa se a thread está esperando outra thread terminar
        if(currentThread->waitingThread != NULL){
            // Testa se a thread que ela está esperando ainda existe, para contornar o caso em que a thread já terminou
            if(existThread(currentThread->waitingThread)){
                dlist_push_right(readyThreadsList, currentThread);
                continue;
            }
            else{
                currentThread->waitingThread = NULL;
            }
        }
        swapcontext(&manager->context, &currentThread->context);
    }
}

/* `dccthread_init` initializes any state necessary for the
 * threadling library and starts running `func`.  this function
 * never returns. */
void dccthread_init(void (*func)(int), int param){
    // Inicializa listas de threads 
    readyThreadsList = dlist_create();
    sleepingThreadsList = dlist_create();

    
    // Inicializa a thread manager
    manager = (dccthread_t *)malloc(sizeof(dccthread_t));
    strcpy(manager->name, "manager");
    getcontext(&manager->context);
    manager->context.uc_link = NULL;
    manager->context.uc_stack.ss_sp = malloc( THREAD_STACK_SIZE );
    manager->context.uc_stack.ss_size = THREAD_STACK_SIZE;
    manager->context.uc_stack.ss_flags = 0;

    manager->waitingThread = NULL;


    makecontext(&manager->context, (void*)managerFunction, 1, param);

    // Inicializa thread principal
    mainThread = dccthread_create("main", func, param);

    // Bloqueia sinais de sleep e preempção para a thread manager
    sigemptyset(&timerMask);
	sigaddset(&timerMask, SIGRTMIN);
	sigaddset(&timerMask, SIGRTMAX);
    sigprocmask(SIG_SETMASK, &timerMask, NULL);

    sigemptyset(&sleepMask);
    sigaddset(&sleepMask, SIGRTMAX);
    manager->context.uc_sigmask = timerMask;

    startTimer();

    setcontext(&manager->context);

    exit(0);
};

/* on success, `dccthread_create` allocates and returns a thread
 * handle.  returns `NULL` on failure.  the new thread will execute
 * function `func` with parameter `param`.  `name` will be used to
 * identify the new thread. */
dccthread_t * dccthread_create(const char *name,void (*func)(int ), int param){

    sigprocmask(SIG_BLOCK, &timerMask, NULL);

    //cria nova thread e aloca espaço para ela
    dccthread_t* newThread; 
    newThread = (dccthread_t*) malloc(sizeof(dccthread_t));

    getcontext(&newThread->context);
    newThread->context.uc_link = &manager->context;
    newThread->context.uc_stack.ss_sp = malloc( THREAD_STACK_SIZE );
    newThread->context.uc_stack.ss_size = THREAD_STACK_SIZE;
    newThread->context.uc_stack.ss_flags = 0;

    newThread->waitingThread = NULL;

    strcpy(newThread->name, name );

    // Garantir que para a nova Thread nenhum sinal está bloqueado
    sigemptyset(&newThread->context.uc_sigmask);

    dlist_push_right(readyThreadsList,newThread);
    //adiona thread a lista de prontas para execução
    makecontext(&newThread->context, (void*) func, 1, param);

    sigprocmask(SIG_UNBLOCK, &timerMask, NULL);

    return newThread;   
};

/* `dccthread_yield` will yield the CPU (from the current thread to
 * another). */
void dccthread_yield(void){

    sigprocmask(SIG_BLOCK, &timerMask, NULL);

    // Tira a thread executada no momento e chama a gerente
    dccthread_t* currentThread = dccthread_self();
    dlist_push_right(readyThreadsList, currentThread);
    swapcontext(&currentThread->context, &manager->context);

    sigprocmask(SIG_UNBLOCK, &timerMask, NULL);
};

/* `dccthread_exit` terminates the current thread, freeing all
 * associated resources. */
void dccthread_exit(void){

    sigprocmask(SIG_BLOCK, &timerMask, NULL);

    dccthread_t* currentThread = dccthread_self();
    free(currentThread);

    setcontext(&manager->context);

    sigprocmask(SIG_UNBLOCK, &timerMask, NULL);
}

/* `dccthread_wait` blocks the current thread until thread `tid`
 * terminates. */
void dccthread_wait(dccthread_t *tid){

    sigprocmask(SIG_BLOCK, &timerMask, NULL);

    // Checando se a thread existe
    if(!existThread(tid)) return;

    currentThread = dccthread_self();
    currentThread->waitingThread = tid;
    
    dlist_push_right(readyThreadsList,currentThread);
    swapcontext(&currentThread->context, &manager->context);

    sigprocmask(SIG_UNBLOCK, &timerMask, NULL);
};

// Definindo comparação da dccthread com base no dlist.c
int cmp(const void *e1, const void *e2, void *userdata) {
    return e1 != e2;
};

// Retorna a thread que está 'dormindo'para fila de pronto
void sleepHandler(int signum, siginfo_t *si, void *context){
    dccthread_t* slpThread = (dccthread_t *)si->si_value.sival_ptr;
    dlist_find_remove(sleepingThreadsList, slpThread, cmp, NULL);
    dlist_push_right(readyThreadsList, slpThread);
}

/* `dccthread_sleep` stops the current thread for the time period
 * specified in `ts`. */
void dccthread_sleep(struct timespec ts){

    sigprocmask(SIG_BLOCK, &timerMask, NULL);

    dccthread_t* currentThread = dccthread_self();

    // Setando o que fazer com o sinal gerado quando for para "acordar"
    sleepAction.sa_sigaction = sleepHandler;
    sleepAction.sa_flags = SA_SIGINFO; // passar informações extras do sinal par ao handler
    sleepAction.sa_mask = timerMask; // Bloquear sinais de preempção durante o handler de sleep
    sigaction(SIGRTMAX, &sleepAction, NULL);

    // Infos sobre o sinal que deve ser enviado
    sleepEvent.sigev_notify = SIGEV_SIGNAL;
    sleepEvent.sigev_value.sival_ptr = currentThread;
    sleepEvent.sigev_signo = SIGRTMAX; // Sinal numérico a ser enviado
    timer_create(CLOCK_REALTIME, &sleepEvent, &sleepTimer);

    // Definindo o timer
    sleepIts.it_value = ts;
    timer_settime(sleepTimer, 0, &sleepIts, NULL);

    dlist_push_right(sleepingThreadsList, currentThread);
    swapcontext(&currentThread->context, &manager->context);

    sigprocmask(SIG_UNBLOCK, &timerMask, NULL);
};

/* `dccthread_name` returns a pointer to the string containing the
 * name of thread `tid`.  the returned string is owned and managed
 * by the library. */
const char * dccthread_name(dccthread_t *tid){
    return tid->name;
};

/* `dccthread_self` returns the current thread's handle. */
dccthread_t * dccthread_self(void){
    return currentThread;
};