#ifndef INCLUDED__BIT_FLOW__TIMEOUT__HELPER__H
#define INCLUDED__BIT_FLOW__TIMEOUT__HELPER__H

/* FILE:        TimeoutHelper.h
 * DATE:        9/13/2019
 * AUTHOR:      Jeremy Greene
 * COMPANY:     BitFlow, Inc.
 * COPYRIGHT:   Copyright (C) 2019, BitFlow, Inc.
 * DESCRIPTION: Preprocessor macros used to simplify the implementation of
 *              functions that take a timeout value.
 */

#include <ctime>

#include "BFLogIODef.h"

#define TIME_NOW() \
    ((BFLOGIO_TIMEOUT_UINT)(clock() * 1000 / CLOCKS_PER_SEC))

#define TIMEOUT_START(PERIOD_MS, INFINITE_PERIOD) \
    const BFLOGIO_TIMEOUT_UINT bfTimeoutHelper_timeoutAt = TIME_NOW() + PERIOD_MS; \
    const bool bfTimeoutHelper_isInfinite = (PERIOD_MS == INFINITE_PERIOD)

#define TIMEOUT_IS_INFINITE() \
    (bfTimeoutHelper_isInfinite)

#define TIMEOUT_PASSED() \
    (!TIMEOUT_IS_INFINITE() && TIME_NOW() >= bfTimeoutHelper_timeoutAt)

#define TIMEOUT_REMAINING(INFINITE_LCL) \
    (TIMEOUT_IS_INFINITE() ? INFINITE_LCL : [bfTimeoutHelper_timeoutAt]{ const BFLOGIO_TIMEOUT_UINT timeNow = TIME_NOW(); return timeNow >= bfTimeoutHelper_timeoutAt ? 0 : bfTimeoutHelper_timeoutAt - timeNow; }())

#endif // INCLUDED__BIT_FLOW__TIMEOUT__HELPER__H
