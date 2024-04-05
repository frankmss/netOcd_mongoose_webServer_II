
// compile cmd: gcc test_sdl_thread_lock.c -o test_sdl_thread_lock -lSDL2
// -std=gnu99
#include <SDL2/SDL_mutex.h>
#include <SDL2/SDL_thread.h>

// The protective mutex
SDL_mutex* gBufferLock = NULL;

// The conditions
SDL_cond* gCanProduce = NULL;
SDL_cond* gCanConsume = NULL;

// The "data buffer"
int gData = -1;

void produce() {
  // Lock
  SDL_LockMutex(gBufferLock);
  printf("produce lock ... 1\n");
  // If the buffer is full
  // if (gData != -1) {
  //   // Wait for buffer to be cleared

  //   printf(
  //       "\nProducer encountered full buffer, waiting for consumer to empty "
  //       "buffer...\n");
  //   SDL_CondWait(gCanProduce, gBufferLock);
  // }
  //SDL_CondWait(gCanProduce, gBufferLock);
  // Fill and show buffer
  //SDL_Delay(rand() % 1000);
  gData = rand() % 255;
  printf("Produced %d\n", gData);

  // Unlock
  SDL_UnlockMutex(gBufferLock);
  printf("produce lock ... 2\n\n");
  // Signal consumer
  //SDL_CondSignal(gCanConsume);
}

void consume() {
  // Lock
  SDL_LockMutex(gBufferLock);
  printf("consume lock 1\n");
  // If the buffer is empty
  // if (gData == -1) {
  //   // Wait for buffer to be filled
  //   printf(
  //       "\nConsumer encountered empty buffer, waiting for producer to fill "
  //       "buffer...\n");
  //   SDL_CondWait(gCanConsume, gBufferLock);
  // }
  //SDL_CondWait(gCanConsume, gBufferLock);
  // Show and empty buffer
  printf("Consumed %d\n", gData);
  gData = -1;
  //SDL_Delay(rand() % 1000);
  // Unlock
  printf("consume lock 2\n\n");
  SDL_UnlockMutex(gBufferLock);

  // Signal producer
  //SDL_CondSignal(gCanProduce);
}

int producer(void* data) {
  printf("\nProducer started...\n");

  // Seed thread random
  srand(SDL_GetTicks());

  // Produce
  for (int i = 0; i < 50; ++i) {
    // Wait
    //SDL_Delay(rand() % 1000);

    // Produce
    produce();
  }

  printf("\nProducer finished!\n");

  return 0;
}

int consumer(void* data) {
  printf("\nConsumer started...\n");

  // Seed thread random
  srand(SDL_GetTicks());

  for (int i = 0; i < 50; ++i) {
    // Wait
    //SDL_Delay(rand() % 1000);

    // Consume
    consume();
  }

  printf("\nConsumer finished!\n");

  return 0;
}

void main(void) {
  
  SDL_CreateThread(consumer, "consumer", NULL);
  //SDL_Delay( 10);
  SDL_CreateThread(producer, "producer", NULL);
  // Create the mutex
  gBufferLock = SDL_CreateMutex();

  // Create conditions
  gCanProduce = SDL_CreateCond();
  gCanConsume = SDL_CreateCond();
  SDL_Delay(200 * 1000);
}