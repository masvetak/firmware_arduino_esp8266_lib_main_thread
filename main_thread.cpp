#include "main_thread.hpp"

#define FILE_NAME_MAIN_THREAD   "\e[32m[MainThread]\e[0m"

MainThread::MainThread() {
    Serial.printf("%s constructor\n\r", FILE_NAME_MAIN_THREAD);
    resetCallbacks();
    resetAsyncDelay();
    resetAsyncFunctions();
}

void MainThread::registerCallback(void (*onCallback)(), uint64_t minTimeout, uint64_t delayStartupTimeout, const char *name) {
    uint8_t index;
    uint8_t cbNameSize = sizeof(_callbackList[_callbackCount].name);
    bool bCbExisted = false;

    if (_callbackCount == _maxCallbacks) {
        // Log_printInfo(FILE_NAME, "registerCallback list full, %s rejected\r\n", name);
        return;
    }

    for (index = 0; index < _callbackCount; index++) {
        if (!memcmp(&name[0], &_callbackList[index].name[0], cbNameSize - 1)) {
            bCbExisted = true;
            break;
        }
    }

    _callbackList[index].startTimestamp = Scheduler::getInstance().getMillis();
    _callbackList[index].minTimeout = minTimeout;
    _callbackList[index].delayStartupTimeout = delayStartupTimeout;
    _callbackList[index].callback = onCallback;
    _callbackList[index].bSuspended = false;

    memcpy(&_callbackList[index].name[0], &name[0], cbNameSize);
    _callbackList[index].name[cbNameSize - 1] = '\0';

    // Log_printInfo(FILE_NAME, "registerCallback %s: %d ms\r\n", _callbackList[index].name, minTimeout);

    if (!bCbExisted) {
        ++_callbackCount;
    }
}

void MainThread::registerCallback(void (*onCallback)(), uint64_t minTimeout, const char *name) {
    registerCallback(onCallback, minTimeout, 0, name);
}

void MainThread::execute() {
    uint64_t startTimestamp = Scheduler::getInstance().getMillis();
    executeCallbacks();
    executeAsyncDelay();
    executeAsyncFunction();
    uint64_t miliPassed = Scheduler::getInstance().millisPassed(startTimestamp);
    if (miliPassed > _alterMillisPassed) {
        //Log_printInfo(FILE_NAME, TBR "Total callback duration of %d ms\n" RST, (uint32_t)miliPassed);
    }
}

void MainThread::executeCallbacks() {
    for (uint8_t index = 0; index < _callbackCount; index++) {
        MainCallback *ref = &_callbackList[index];
        if (ref->callback != nullptr && !ref->bSuspended &&
            Scheduler::getInstance().millisPassed(ref->startTimestamp) >= ref->minTimeout &&
                Scheduler::getInstance().millisPassed(Scheduler::getInstance().getMillisOffset()) >= ref->delayStartupTimeout
                ) {
            currentCallback = ref;
            ref->startTimestamp = Scheduler::getInstance().getMillis();
            ref->callback();
            uint64_t miliPassed = Scheduler::getInstance().millisPassed(ref->startTimestamp);
            if (miliPassed >= _alterMillisPassed) {
                // Log_printInfo(FILE_NAME, "callback %s has duration of %d ms\n", ref->name, (uint32_t)miliPassed);
            }
        }
    }
}

void MainThread::resetCallbacks() {
    for (uint8_t index = 0; index < _maxCallbacks; index++) {
        MainCallback *ref = &_callbackList[index];
        ref->callback = nullptr;
        ref->startTimestamp = Scheduler::getInstance().getMillis();
    }
}

void MainThread::setAltertMillisPassed(uint64_t millisPassed) {
    _alterMillisPassed = millisPassed;
}

void MainThread::suspendCallback(const char *name, bool bFlag) {
    uint8_t index;
    uint8_t cbNameSize = sizeof(_callbackList[_callbackCount].name);

    for (index = 0; index < _callbackCount; index++) {
        if (!memcmp(&name[0], &_callbackList[index].name[0], cbNameSize - 1)) {
            if (!bFlag) {
                _callbackList[index].startTimestamp = Scheduler::getInstance().getMillis();;
            }
            _callbackList[index].bSuspended = bFlag;
        }
    }
}

void MainThread::changeCallbackMinTimeout(void (*onCallback)(), uint64_t minTimeout) {
    uint8_t index;
    bool callbackMatch = false;
    for (index = 0; index < _callbackCount; index++) {
        if (_callbackList[index].callback == onCallback){
            callbackMatch = true;
            break;
        }
    }
    if (callbackMatch){
        _callbackList[index].minTimeout = minTimeout;
        // Log_printInfo(FILE_NAME, "changeCallbackMinTimeout successful for %s to %d\n", _callbackList[index].name, _callbackList[index].minTimeout);
    } else {
        // Log_printInfo(FILE_NAME, "changeCallbackMinTimeout failed, NO MATCH\n");
    }
}

uint8_t MainThread::getCbCount() {
    return _callbackCount;
}

void MainThread::printCurrentCallback(){
    // uint64_t miliPassed = AppTimer::getInstance().millisPassed(currentCallback->startTimestamp);
    // Log_printInfo(FILE_NAME, "printCurrentCallback %s current duration is %d ms\n", currentCallback->name, (uint32_t)miliPassed);
}

void MainThread::resetAsyncDelay() {
    for (uint8_t i = 0; i < _maxAsyncDelays; i++) {
        asyncDelayRefList[i].isSet = false;
    }
}

void MainThread::asyncDelay(uint32_t delay, void (*onAsyncDelayComplete)(bool isSuccessful)) {
    uint8_t index;
    for (index = 0; index < _maxAsyncDelays; index++) {
        if (!asyncDelayRefList[index].isSet) break;
    }
    if (index >= _maxAsyncDelays) {
        onAsyncDelayComplete(false);
        return;
    }

    asyncDelayRefList[index].isSet = true;
    asyncDelayRefList[index].asyncDelayStart = Scheduler::getInstance().getMillis();
    asyncDelayRefList[index].asyncDelayTimeout = delay;
    asyncDelayRefList[index].asyncDelayCallback = onAsyncDelayComplete;
}

void MainThread::executeAsyncDelay() {
    for (uint8_t index = 0; index < _maxAsyncDelays; index++) {
        if (asyncDelayRefList[index].isSet) {
            AsyncDelayRef *ref = &asyncDelayRefList[index];
            if (ref->asyncDelayCallback != nullptr &&
                Scheduler::getInstance().millisPassed(ref->asyncDelayStart) > ref->asyncDelayTimeout) {
                ref->asyncDelayStart = 0;
                ref->asyncDelayTimeout = 0;

                asyncDelayRefList[index].isSet = false;
                void (*callback)(bool isSuccessful) = ref->asyncDelayCallback;
                ref->asyncDelayCallback = nullptr;
                callback(true);
            }
        }
    }
}

void MainThread::resetAsyncFunctions() {
    for (uint8_t i = 0; i < _maxAsyncFunctions; i++) {
        asyncFunctionRefList[i].isSet = false;
    }
}

void MainThread::registerAsyncFunction(void (*functionStartCallback)(),
                                    void (*functionEndCallback)(bool success),
                                    uint64_t startDelay, uint64_t timeout, const char *name) {
    for (uint8_t i = 0; i < _maxAsyncFunctions; i++) {
        if (asyncFunctionRefList[i].isSet &&
            asyncFunctionRefList[i].functionStartCallback == functionStartCallback &&
            asyncFunctionRefList[i].functionEndCallback == functionEndCallback
                ) {
            //Log_printInfo(FILE_NAME, "registerAsyncFunction %s failed, already registrated\n", name);
            return;
        }
    }
    uint8_t index;
    for (index = 0; index < _maxAsyncFunctions; index++) {
        if (!asyncFunctionRefList[index].isSet) break;
    }
    if (index >= _maxAsyncFunctions) {
        //Log_printInfo(FILE_NAME, "registerAsyncFunction %s failed\n", name);
        functionEndCallback(false);
        return;
    }

    asyncFunctionRefList[index].isSet = true;
    asyncFunctionRefList[index].isInProgress = false;
    asyncFunctionRefList[index].timestamp = Scheduler::getInstance().getMillis();
    asyncFunctionRefList[index].startDelay = startDelay;
    asyncFunctionRefList[index].timeout = timeout;
    asyncFunctionRefList[index].functionStartCallback = functionStartCallback;
    asyncFunctionRefList[index].functionEndCallback = functionEndCallback;
    strcpy(asyncFunctionRefList[index].name, name);
    //Log_printInfo(FILE_NAME, "registerAsyncFunction %s success\n", name);
}

void MainThread::executeAsyncFunction() {
    for (uint8_t index = 0; index < _maxAsyncFunctions; index++) {
        if (asyncFunctionRefList[index].isSet) {
            AsyncFunctionRef *ref = &asyncFunctionRefList[index];
            if (!ref->isInProgress && Scheduler::getInstance().millisPassed(ref->timestamp) >= ref->startDelay) {
                //Log_printInfo(FILE_NAME, "executeAsyncFunction %s start\n", ref->name);
                ref->isInProgress = true;
                ref->functionStartCallback();
            } else {
                if (Scheduler::getInstance().millisPassed(ref->timestamp) >= ref->timeout) {
                    //Log_printInfo(FILE_NAME, "executeAsyncFunction %s end: TIMEOUT\n", ref->name);
                    ref->functionEndCallback(false);
                }
            }
        }
    }
}

void MainThread::deregisterAsyncFunction(void (*functionStartCallback)(),
                                        void (*functionEndCallback)(bool success)) {
    for (uint8_t i = 0; i < _maxAsyncFunctions; i++) {
        if (asyncFunctionRefList[i].isSet &&
            asyncFunctionRefList[i].functionStartCallback == functionStartCallback &&
            asyncFunctionRefList[i].functionEndCallback == functionEndCallback
                ) {
            //Log_printInfo(FILE_NAME, "deregisterAsyncFunction %s\n", asyncFunctionRefList[i].name);
            asyncFunctionRefList[i].isSet = false;
        }
    }
}