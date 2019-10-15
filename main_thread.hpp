/* Define to prevent recursive inclusion ------------------------------------ */
#ifndef __MAIN_THREAD_HPP__
#define __MAIN_THREAD_HPP__

#include <cstring>

#include "scheduler.hpp"

struct MainCallback {
    uint64_t startTimestamp;
    uint64_t minTimeout;
    uint64_t delayStartupTimeout;
    char name[20];
    bool bSuspended;

    void (*callback)();
};

struct AsyncFunctionRef {
    uint64_t timestamp;
    uint64_t startDelay;
    uint64_t timeout;

    void (*functionStartCallback)();

    void (*functionEndCallback)(bool isSuccessful);

    bool isSet;
    bool isInProgress;
    char name[20];
};

struct AsyncDelayRef {
    uint64_t asyncDelayStart;
    uint64_t asyncDelayTimeout;
    bool isSet;

    void (*asyncDelayCallback)(bool isSuccessful);
};

class MainThread {

    private:
        // constructor
        MainThread();

        // singleton (C++ 03) - unimplemented methods
        MainThread(MainThread const&);
        void operator=(MainThread const&);

        // singleton (C++ 11) - deleted methods
        // public:
        // MainThread(MainThread const&) = delete;
        // void operator=(MainThread const&) = delete;

        static const uint8_t _maxCallbacks = 20;
        uint8_t _callbackCount = 0;
        uint64_t _alterMillisPassed = 2;

        struct MainCallback _callbackList[_maxCallbacks];
        MainCallback *currentCallback = nullptr;

        static const uint8_t _maxAsyncDelays = 10;
        struct AsyncDelayRef asyncDelayRefList[_maxAsyncDelays];

        static const uint8_t _maxAsyncFunctions = 10;
        struct AsyncFunctionRef asyncFunctionRefList[_maxAsyncFunctions];

    public:
        // singleton
        static MainThread& getInstance() {
            static MainThread instance;
            return instance;
        }

        void registerCallback(void (*onCallback)(), uint64_t minTimeout, uint64_t delayStartupTimeout, const char *name);

        void registerCallback(void (*onCallback)(), uint64_t minTimeout, const char *name);

        void execute();

        void executeCallbacks();

        void resetCallbacks();

        void setAltertMillisPassed(uint64_t millisPassed);

        void suspendCallback(const char *name, bool bFlag);

        void changeCallbackMinTimeout(void (*onCallback)(), uint64_t minTimeout);

        uint8_t getCbCount();

        void printCurrentCallback();

        void resetAsyncDelay();

        void asyncDelay(uint32_t delay, void (*onAsyncDelayComplete)(bool isSuccessful));

        void executeAsyncDelay();

        void resetAsyncFunctions();

        void registerAsyncFunction(void (*functionStartCallback)(),
                                   void (*functionEndCallback)(bool success),
                                   uint64_t startDelay, uint64_t timeout, const char *name);

        void executeAsyncFunction();

        void deregisterAsyncFunction(void (*functionStartCallback)(),
                                     void (*functionEndCallback)(bool success));
};

/* Define to prevent recursive inclusion ------------------------------------ */
#endif //__MAIN_THREAD_HPP__