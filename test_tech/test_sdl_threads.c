// compile cmd: gcc test_sdl_threads.c -o test_sdl_threads -lSDL2
#include <SDL2/SDL_thread.h>

#include <SDL2/SDL_mutex.h>

int potty = 0;
int gotta_go;

int thread_func(void *data)
{
    SDL_mutex *lock = (SDL_mutex *)data;
    int times_went;
    int lockStatus;

    times_went = 0;
    while ( gotta_go ) {
        do {
            lockStatus = SDL_mutexP(lock);    /* Lock  the potty */
        }while(lockStatus!=0);
        
        ++potty;
        SDL_threadID tid = SDL_ThreadID();
        printf("Thread %ld using the potty\n", tid);
        // if ( potty > 1 ) {
        //     printf("Uh oh, somebody else is using the potty!\n");
        // }
        --potty;
        SDL_mutexV(lock);
        ++times_went;
    }
    printf("Yep\n");
    return(times_went);
}

void main(voi)
{
    const int progeny = 5;
    SDL_Thread *kids[progeny];
    SDL_mutex  *lock;
    int i, lots;

    /* Create the synchronization lock */
    lock = SDL_CreateMutex();

    gotta_go = 1;
    for ( i=0; i<progeny; ++i ) {
        char threadName[10];
        sprintf(threadName,"thread%d",i);
        kids[i] = SDL_CreateThread(thread_func, threadName,&lock);
    }

    SDL_Delay(5*1000);
    SDL_mutexP(lock);
    printf("Everybody done?\n");
    gotta_go = 0;
    SDL_mutexV(lock);

    for ( i=0; i<progeny; ++i ) {
        SDL_WaitThread(kids[i], &lots);
        printf("Thread %d used the potty %d times\n", i+1, lots);
    }
    SDL_DestroyMutex(lock);
}