#include <condition_variable>
#include <csignal>
#include <iostream>
#include <mutex>
#include <ostream>
#include <pthread.h>

#include "mealy.hpp"

using namespace std;

enum State { INIT, A, B, C, D, E };

extern "C" void DUMP_GLOB(void);

extern "C" {
extern char __data_start[]; // Start of data section
extern char _edata;         // End of data section
extern char __bss_start;    // Start of bss section
extern char _end[];         // End of bss section

// Some extract for reference
// 00000000000050b0 D __data_start
// 00000000000050b0 W data_start
// 00000000000050b8 D __dso_handle
// 00000000000050c0 V DW.ref.__gxx_personality_v0
// 00000000000050c8 B __bss_start
// 00000000000050c8 D _edata
// 00000000000050c8 D __TMC_END__
// 00000000000050d0 b _ZL14resetCondition
// 0000000000005100 b _ZL7machine
// 0000000000005104 b _ZL12resetPending
// 0000000000005108 b _ZL10stateMutex
// 0000000000005130 b _ZL12GLOBAL_STATE
// 0000000000005138 B _end

void checkRangeGlob(void *ptr, bool isRead) {
  // We care only about changes
  if(isRead)
    return;
      
  // Check if the address is within the global regions (.data or .bss)
  if ((ptr >= &__data_start && ptr < &_edata)) {
    cout << "-D: " << std::hex << ((char *)ptr - (char*)&__data_start) << endl;
  } else if (ptr >= &__bss_start && ptr < &_end) {
    cout << "-B: " << std::hex << "0x" << ((char *)ptr - (char*)&__bss_start) << endl;
  }
}
}

[[maybe_unused]]
static int resets_cnt = 0;
int abc;
static std::mutex stateMutex;
static std::condition_variable resetCondition;
static bool resetPending = false; // Flag to indicate pending reset


static struct programstate GLOBAL_STATE = {
    .has_reached_A = false,
    .has_reached_B = false,
    .inside_state = {.is_inside = false},
};

class MealyMachine {
public:
  MealyMachine() : currentState(INIT) {}

  void processInput(char input) {
    std::unique_lock<std::mutex> lock(stateMutex);
    resetCondition.wait(
        lock, [] { return !resetPending; }); // Wait until no reset is pending


    // Process input based on the current state
    switch (currentState) {
    case INIT:
      handleInitState(input);
      break;
    case A:
      handleAState(input);
      break;
    case B:
      handleBState(input);
      break;
    case C:
      handleCState(input);
      break;
    case D:
      handleDState(input);
      break;
    case E:
      handleEState(input);
      break;
    default:
      reset();
      break;
    }
  }

  void resetHard() {
    currentState = INIT;
    GLOBAL_STATE.has_reached_A = false;
    GLOBAL_STATE.has_reached_B = false;
    GLOBAL_STATE.inside_state.is_inside = true;
  }

private:
  State currentState;

  void handleInitState(char input) {
    switch (input) {
    case 'a':
      GLOBAL_STATE.has_reached_A = true;
      currentState = A;
      cout << "1" << endl << flush;
      break;
    case 'b':
      GLOBAL_STATE.has_reached_B = true;
      currentState = B;
      cout << "2" << endl << flush;
      break;
    default:
      reset();
      break;
    }
  }

  void handleAState(char input) {
    switch (input) {
    case 'c':
      currentState = C;
      cout << "3" << endl << flush;
      ;
      break;
    case 'd':
      currentState = D;
      cout << "4" << endl << flush;
      ;
      break;
    default:
      reset();
      break;
    }
  }

  void handleBState(char input) {
    switch (input) {
    case 'e':
      currentState = E;
      cout << "5" << endl << flush;
      ;
      break;
    default:
      reset();
      break;
    }
  }

  void handleCState(char input) {
    switch (input) {
    case 'f':
      GLOBAL_STATE.has_reached_A = true;
      currentState = A;
      cout << "6" << endl << flush;
      ;
      break;
    default:
      reset();
      break;
    }
  }

  void handleDState(char input) {
    switch (input) {
    case 'g':
      GLOBAL_STATE.has_reached_B = true;
      currentState = B;
      cout << "7" << endl << flush;
      ;
      break;
    default:
      reset();
      break;
    }
  }

  void handleEState(char input) {
    switch (input) {
    case 'h':
      currentState = C;
      cout << "8" << endl << flush;
      ;
      break;
    default:
      reset();
      break;
    }
  }

  void reset() {
    cout << "X" << endl << flush;
    ;
    resetHard();
  }
};

static MealyMachine machine;

// Signal-handling thread function
void *signalHandlerThread(void *arg) {
  sigset_t *set = static_cast<sigset_t *>(arg);
  int sig;

  while (true) {
    // Wait for the signal (SIGUSR1)
    if (sigwait(set, &sig) == 0 && sig == SIGUSR1) {
      std::lock_guard<std::mutex> lock(stateMutex);
      resetPending = true;

      // DUMP_GLOB();
      machine.resetHard();

      resetPending = false;
      resetCondition.notify_all(); // Wake up main thread for processing
      cout << "OK" << endl << flush;
    }
  }
  return nullptr;
}

int main() {
  // Mask SIGUSR1 in all threads
  sigset_t set;
  sigemptyset(&set);
  sigaddset(&set, SIGUSR1);
  pthread_sigmask(SIG_BLOCK, &set, nullptr);

  // Start the signal-handling thread
  pthread_t sigThread;
  pthread_create(&sigThread, nullptr, signalHandlerThread, &set);

  // Main input loop
  char input;
  while (cin >> input) {
    machine.processInput(input);
    // DUMP_GLOB();
  }

  pthread_join(sigThread, nullptr);
  return 0;
}
