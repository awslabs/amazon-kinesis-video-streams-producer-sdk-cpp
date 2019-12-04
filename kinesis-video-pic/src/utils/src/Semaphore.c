#include "Include_i.h"

/**
 * Create a semaphore object
 */
STATUS semaphoreCreate(UINT32 maxPermits, PSEMAPHORE_HANDLE pHandle)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PSemaphore pSemaphore = NULL;

    CHK(pHandle != NULL, STATUS_NULL_ARG);

    CHK_STATUS(semaphoreCreateInternal(maxPermits, &pSemaphore));

    *pHandle = TO_SEMAPHORE_HANDLE(pSemaphore);

CleanUp:

    if (STATUS_FAILED(retStatus)) {
        semaphoreFreeInternal(&pSemaphore);
    }

    LEAVES();
    return retStatus;
}

/**
 * Frees and de-allocates the semaphore
 */
STATUS semaphoreFree(PSEMAPHORE_HANDLE pHandle)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PSemaphore pSemaphore;

    CHK(pHandle != NULL, STATUS_NULL_ARG);

    // Get the client handle
    pSemaphore = FROM_SEMAPHORE_HANDLE(*pHandle);

    CHK_STATUS(semaphoreFreeInternal(&pSemaphore));

    // Set the handle pointer to invalid
    *pHandle = INVALID_SEMAPHORE_HANDLE_VALUE;

CleanUp:

    LEAVES();
    return retStatus;
}

STATUS semaphoreAcquire(SEMAPHORE_HANDLE handle, UINT64 timeout)
{
    return semaphoreAcquireInternal(FROM_SEMAPHORE_HANDLE(handle), timeout);
}

STATUS semaphoreRelease(SEMAPHORE_HANDLE handle)
{
    return semaphoreReleaseInternal(FROM_SEMAPHORE_HANDLE(handle));
}

STATUS semaphoreLock(SEMAPHORE_HANDLE handle)
{
    return semaphoreSetLockInternal(FROM_SEMAPHORE_HANDLE(handle), TRUE);
}

STATUS semaphoreUnlock(SEMAPHORE_HANDLE handle)
{
    return semaphoreSetLockInternal(FROM_SEMAPHORE_HANDLE(handle), FALSE);
}

STATUS semaphoreWaitUntilClear(SEMAPHORE_HANDLE handle, UINT64 timeout)
{
    return semaphoreWaitUntilClearInternal(FROM_SEMAPHORE_HANDLE(handle), timeout);
}

/////////////////////////////////////////////////////////////////////////////////
// Internal operations
/////////////////////////////////////////////////////////////////////////////////
STATUS semaphoreCreateInternal(UINT32 maxPermits, PSemaphore* ppSemaphore)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PSemaphore pSemaphore = NULL;

    CHK(ppSemaphore != NULL, STATUS_NULL_ARG);
    CHK(maxPermits > 0, STATUS_INVALID_ARG);
    CHK(NULL != (pSemaphore = (PSemaphore) MEMCALLOC(1, SIZEOF(Semaphore))), STATUS_NOT_ENOUGH_MEMORY);
    pSemaphore->maxPermitCount = maxPermits;
    ATOMIC_STORE(&pSemaphore->permitCount, maxPermits);

    pSemaphore->semaphoreLock = MUTEX_CREATE(FALSE);
    CHK(IS_VALID_MUTEX_VALUE(pSemaphore->semaphoreLock), STATUS_INVALID_OPERATION);
    pSemaphore->semaphoreNotify = CVAR_CREATE();
    CHK(IS_VALID_CVAR_VALUE(pSemaphore->semaphoreNotify), STATUS_INVALID_OPERATION);

    pSemaphore->drainedLock = MUTEX_CREATE(FALSE);
    CHK(IS_VALID_MUTEX_VALUE(pSemaphore->drainedLock), STATUS_INVALID_OPERATION);
    pSemaphore->drainedNotify = CVAR_CREATE();
    CHK(IS_VALID_CVAR_VALUE(pSemaphore->drainedNotify), STATUS_INVALID_OPERATION);

CleanUp:

    CHK_LOG_ERR(retStatus);

    if (STATUS_FAILED(retStatus)) {
        semaphoreFreeInternal(&pSemaphore);
    }

    if (ppSemaphore != NULL) {
        *ppSemaphore = pSemaphore;
    }

    LEAVES();
    return retStatus;
}

STATUS semaphoreFreeInternal(PSemaphore* ppSemaphore)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    PSemaphore pSemaphore = NULL;
    UINT64 waitTime = 0;

    CHK(ppSemaphore != NULL, STATUS_NULL_ARG);

    pSemaphore = *ppSemaphore;
    CHK(pSemaphore != NULL, retStatus);

    // Lock the semaphore first
    ATOMIC_STORE_BOOL(&pSemaphore->locked, TRUE);

    // Indicate a shutdown
    ATOMIC_STORE_BOOL(&pSemaphore->shutdown, TRUE);

    // Notify all to unblock
    if (IS_VALID_CVAR_VALUE(pSemaphore->semaphoreNotify) && IS_VALID_CVAR_VALUE(pSemaphore->drainedNotify)) {
        CVAR_BROADCAST(pSemaphore->semaphoreNotify);
        CVAR_BROADCAST(pSemaphore->drainedNotify);
    }

    // We will use a sort of spin-lock to await for termination
    while (ATOMIC_LOAD(&pSemaphore->permitCount) != pSemaphore->maxPermitCount &&
        waitTime <= SEMAPHORE_SHUTDOWN_TIMEOUT) {
        THREAD_SLEEP(SEMAPHORE_SHUTDOWN_SPINLOCK_SLEEP_DURATION);
        waitTime += SEMAPHORE_SHUTDOWN_SPINLOCK_SLEEP_DURATION;
    }

    if (IS_VALID_MUTEX_VALUE(pSemaphore->semaphoreLock)) {
        MUTEX_FREE(pSemaphore->semaphoreLock);
    }

    if (IS_VALID_MUTEX_VALUE(pSemaphore->drainedLock)) {
        MUTEX_FREE(pSemaphore->drainedLock);
    }

    if (IS_VALID_CVAR_VALUE(pSemaphore->semaphoreNotify)) {
        CVAR_FREE(pSemaphore->semaphoreNotify);
    }

    if (IS_VALID_CVAR_VALUE(pSemaphore->drainedNotify)) {
        CVAR_FREE(pSemaphore->drainedNotify);
    }

    MEMFREE(pSemaphore);

    *ppSemaphore = NULL;

CleanUp:

    CHK_LOG_ERR(retStatus);
    LEAVES();
    return retStatus;
}

STATUS semaphoreAcquireInternal(PSemaphore pSemaphore, UINT64 timeout)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    SIZE_T existingCount;
    BOOL locked = FALSE, acquireFailed = FALSE;

    CHK(pSemaphore != NULL, STATUS_NULL_ARG);
    CHK(!ATOMIC_LOAD_BOOL(&pSemaphore->shutdown), STATUS_SEMAPHORE_OPERATION_AFTER_SHUTDOWN);
    CHK(!ATOMIC_LOAD_BOOL(&pSemaphore->locked), STATUS_SEMAPHORE_ACQUIRE_WHEN_LOCKED);

    existingCount = ATOMIC_DECREMENT(&pSemaphore->permitCount);

    if ((INT32) existingCount <= 0) {
        // Block till availability
        MUTEX_LOCK(pSemaphore->semaphoreLock);
        locked = TRUE;

        // Make sure we clean up if we fail
        acquireFailed = TRUE;

        // There seem to be no sane way to handle the spurious wake-ups
        CHK_STATUS(CVAR_WAIT(pSemaphore->semaphoreNotify, pSemaphore->semaphoreLock, timeout));

        MUTEX_UNLOCK(pSemaphore->semaphoreLock);
        locked = FALSE;

        // Set the flag indicating we succeeded acquiring.
        acquireFailed = FALSE;
    }

CleanUp:

    CHK_LOG_ERR(retStatus);

    if (locked) {
        MUTEX_UNLOCK(pSemaphore->semaphoreLock);
    }

    if (acquireFailed) {
        ATOMIC_INCREMENT(&pSemaphore->permitCount);
    }

    LEAVES();
    return retStatus;
}

STATUS semaphoreReleaseInternal(PSemaphore pSemaphore)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    SIZE_T existingCount;
    BOOL fixupIncrement = FALSE;

    CHK(pSemaphore != NULL, STATUS_NULL_ARG);
    CHK(!ATOMIC_LOAD_BOOL(&pSemaphore->shutdown), STATUS_SEMAPHORE_OPERATION_AFTER_SHUTDOWN);

    existingCount = ATOMIC_INCREMENT(&pSemaphore->permitCount);
    fixupIncrement = TRUE;
    CHK((INT32) existingCount < (INT32) pSemaphore->maxPermitCount, STATUS_INVALID_OPERATION);

    if ((INT32) existingCount < 0) {
        CHK_STATUS(CVAR_SIGNAL(pSemaphore->semaphoreNotify));
    } else if (existingCount + 1 == pSemaphore->maxPermitCount) {
        CHK_STATUS(CVAR_BROADCAST(pSemaphore->drainedNotify));
    }

    // Make sure we do not fix-up after successfully triggering conditional variables
    fixupIncrement = FALSE;

CleanUp:

    CHK_LOG_ERR(retStatus);

    if (fixupIncrement) {
        ATOMIC_DECREMENT(&pSemaphore->permitCount);
    }

    LEAVES();
    return retStatus;
}

STATUS semaphoreSetLockInternal(PSemaphore pSemaphore, BOOL locked)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;

    CHK(pSemaphore != NULL, STATUS_NULL_ARG);
    CHK(!ATOMIC_LOAD_BOOL(&pSemaphore->shutdown), STATUS_SEMAPHORE_OPERATION_AFTER_SHUTDOWN);
    ATOMIC_STORE_BOOL(&pSemaphore->locked, locked);

CleanUp:

    CHK_LOG_ERR(retStatus);
    LEAVES();
    return retStatus;
}

STATUS semaphoreWaitUntilClearInternal(PSemaphore pSemaphore, UINT64 timeout)
{
    ENTERS();
    STATUS retStatus = STATUS_SUCCESS;
    BOOL locked = FALSE;

    CHK(pSemaphore != NULL, STATUS_NULL_ARG);
    CHK(!ATOMIC_LOAD_BOOL(&pSemaphore->shutdown), STATUS_SEMAPHORE_OPERATION_AFTER_SHUTDOWN);

    MUTEX_LOCK(pSemaphore->drainedLock);
    locked = TRUE;

    while (ATOMIC_LOAD(&pSemaphore->permitCount) != pSemaphore->maxPermitCount) {
        CHK_STATUS(CVAR_WAIT(pSemaphore->drainedNotify, pSemaphore->drainedLock, timeout));
    }

    MUTEX_UNLOCK(pSemaphore->drainedLock);
    locked = FALSE;

CleanUp:

    CHK_LOG_ERR(retStatus);

    if (locked) {
        MUTEX_UNLOCK(pSemaphore->drainedLock);
    }

    LEAVES();
    return retStatus;
}