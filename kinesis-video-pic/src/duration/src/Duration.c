#include "Duration.h"

#define HUNDRED_NANOS_IN_SECOND (UINT64) 10000000ULL

// Per spec, we can have ',' or '.' as the fraction sign
#define FRACTION_SIGN_DOT   '.'
#define FRACTION_SIGN_COMMA ','

/**
 * Parser const declarations
 */
#define PARSER_SECOND_DESIGNATOR 'S'
#define PARSER_SECOND_MULTIPLIER (HUNDRED_NANOS_IN_SECOND * 1ULL)

#define PARSER_MINUTE_DESIGNATOR 'M'
#define PARSER_MINUTE_MULTIPLIER (PARSER_SECOND_MULTIPLIER * 60ULL)

#define PARSER_HOUR_DESIGNATOR 'H'
#define PARSER_HOUR_MULTIPLIER (PARSER_MINUTE_MULTIPLIER * 60ULL)

#define PARSER_DAY_DESIGNATOR 'D'
#define PARSER_DAY_MULTIPLIER (PARSER_HOUR_MULTIPLIER * 24ULL)

#define PARSER_WEEK_DESIGNATOR 'W'
#define PARSER_WEEK_MULTIPLIER (PARSER_DAY_MULTIPLIER * 7ULL)

#define PARSER_MONTH_DESIGNATOR 'M'
/**
 * We are assuming 31 days per month
 */
#define PARSER_MONTH_MULTIPLIER (PARSER_DAY_MULTIPLIER * 31ULL)

#define PARSER_YEAR_DESIGNATOR 'Y'
/**
 * We are assuming 365 days per year
 */
#define PARSER_YEAR_MULTIPLIER (PARSER_DAY_MULTIPLIER * 365ULL)

#define PARSER_DURATION_DESIGNATOR 'P'
#define PARSER_TIME_DESIGNATOR     'T'

typedef struct {
    CHAR designator;
    UINT64 multiplier;
} DurationState;

// Parser duration states - IMPORTANT! Order is important
const DurationState DURATION_STATES[] = {
    {PARSER_YEAR_DESIGNATOR, PARSER_YEAR_MULTIPLIER},     {PARSER_MONTH_DESIGNATOR, PARSER_MONTH_MULTIPLIER},
    {PARSER_WEEK_DESIGNATOR, PARSER_WEEK_MULTIPLIER},     {PARSER_DAY_DESIGNATOR, PARSER_DAY_MULTIPLIER},
    {PARSER_HOUR_DESIGNATOR, PARSER_HOUR_MULTIPLIER},     {PARSER_MINUTE_DESIGNATOR, PARSER_MINUTE_MULTIPLIER},
    {PARSER_SECOND_DESIGNATOR, PARSER_SECOND_MULTIPLIER},
};

#define DURATION_STATES_COUNT (sizeof(DURATION_STATES) / sizeof *(DURATION_STATES))

// Corresponds to the order of the hour entry in the DURATION_STATES array
#define HOUR_DESIGNATOR_ORDER 4

// Parser internal tracking
typedef struct {
    PCHAR pCurPnt;
    PCHAR pStrEnd;
    UINT64 value;
    DOUBLE accValue;
    UINT32 durationState;
    BOOL seenTime;
} ParserState, *PParserState;

#define HAS_MORE_INPUT_DATA(p) ((p)->pCurPnt < (p)->pStrEnd)

/**
 * Internal functions forward declarations
 */
BOOL parseDurationStart(PParserState);
BOOL parseDurationValue(PParserState);
BOOL acceptState(PParserState);
BOOL checkForTimeDesignator(PParserState);
BOOL isDigit(CHAR);
UINT32 getDigit(CHAR);

/**
 * Parses ISO 8601 period string for DASH manifest format
 * Returns the corresponding number of 100 ns
 */
UINT64 parseDuration(PCHAR pPeriod, PCHAR pEnd)
{
    // Validate the input and get the size
    if (NULL == pPeriod) {
        return INVALID_DURATION_VALUE;
    }

    ParserState parserState;

    // Set the initial values
    parserState.pCurPnt = pPeriod;

    // If the pEnd is not specified then we will use STRLEN
    if (pEnd == NULL) {
        parserState.pStrEnd = pPeriod + STRLEN(pPeriod);
    } else {
        parserState.pStrEnd = pEnd;
    }

    parserState.value = 0;
    parserState.accValue = 0.0;
    parserState.durationState = 0; // Start with the year
    parserState.seenTime = FALSE;

    if (!parseDurationStart(&parserState)) {
        return INVALID_DURATION_VALUE;
    }

    // Ensure we have consumed the entire string
    if (parserState.pCurPnt != parserState.pStrEnd) {
        return INVALID_DURATION_VALUE;
    }

    return parserState.value;
}

BOOL parseDurationStart(PParserState pState)
{
    if (!HAS_MORE_INPUT_DATA(pState)) {
        return FALSE;
    }

    // See if we have what we expect in the window
    if (*(pState->pCurPnt) != PARSER_DURATION_DESIGNATOR) {
        return FALSE;
    }

    // Advance the position
    pState->pCurPnt++;

    while (HAS_MORE_INPUT_DATA(pState)) {
        if (!checkForTimeDesignator(pState)) {
            return FALSE;
        }

        if (!parseDurationValue(pState)) {
            return FALSE;
        }

        if (!acceptState(pState)) {
            return FALSE;
        }
    }

    return TRUE;
}

BOOL checkForTimeDesignator(PParserState pState)
{
    // Peek to see what's next
    // If it's time designator then we skip to time
    if (HAS_MORE_INPUT_DATA(pState) && (*(pState->pCurPnt) == PARSER_TIME_DESIGNATOR)) {
        if (pState->seenTime) {
            // Already seen the time designator - error
            return FALSE;
        }

        // Advance the position
        pState->pCurPnt++;
        pState->durationState = HOUR_DESIGNATOR_ORDER;
        pState->seenTime = TRUE;
    }

    return TRUE;
}

BOOL acceptState(PParserState pState)
{
    UINT32 i;

    if (!HAS_MORE_INPUT_DATA(pState)) {
        return FALSE;
    }

    // Get the designator from the window
    CHAR designator = *(pState->pCurPnt);

    // Advance the position before we check
    pState->pCurPnt++;

    // Need to check from the given position in the ordered array
    for (i = pState->durationState; i < DURATION_STATES_COUNT; i++) {
        if (designator == DURATION_STATES[i].designator) {
            pState->durationState = i;

            DOUBLE tempResult = pState->accValue * DURATION_STATES[i].multiplier;
            if (tempResult > (DOUBLE) MAX_UINT64) {
                // Overflow detected
                return FALSE;
            }

            UINT64 storedValue = pState->value;
            pState->value += (UINT64)(tempResult);

            if ((pState->value < storedValue) || (tempResult != 0.0 && ((UINT64) tempResult == 0))) {
                // Overflow detected in addition or casting
                return FALSE;
            }

            return TRUE;
        }
    }

    return FALSE;
}

BOOL parseDurationValue(PParserState pState)
{
    BOOL seenFraction = FALSE;
    BOOL seenDigits = FALSE;
    UINT32 quot = 10;

    if (!HAS_MORE_INPUT_DATA(pState)) {
        return FALSE;
    }

    // Reset the accumulator first
    pState->accValue = 0.0;

    while (pState->pCurPnt < pState->pStrEnd) {
        // Get the char - don't advance the position yet
        CHAR ch = *(pState->pCurPnt);

        if (ch == FRACTION_SIGN_DOT || ch == FRACTION_SIGN_COMMA) {
            // Advance the position
            pState->pCurPnt++;

            if (!seenDigits || seenFraction) {
                // Can't have a fraction sign without any digit before it or if we had seen this prior
                return FALSE;
            }

            // Process the fractional part
            seenFraction = TRUE;

            // Reset whether we have seen digits for later checking the error case like "34.S"
            seenDigits = FALSE;
        } else if (!isDigit(ch)) {
            if (seenFraction && !seenDigits) {
                // This is the invalid case like "34.S"
                return FALSE;
            }

            return TRUE;
        } else {
            // Advance the position
            pState->pCurPnt++;

            // Now we are processing digits
            seenDigits = TRUE;

            // Process the digit
            UINT32 digit = getDigit(ch);
            if (seenFraction) {
                // Process as a decimal fraction digit
                pState->accValue = pState->accValue + ((DOUBLE) digit / quot);
                quot *= 10;
            } else {
                pState->accValue = pState->accValue * 10 + digit;

                if (pState->accValue > (DOUBLE) MAX_UINT64) {
                    // Integer overflow detected
                    return FALSE;
                }
            }
        }
    }

    // This is the error case like "34.5" - no designator at the end
    return FALSE;
}

BOOL isDigit(CHAR ch)
{
    // Process ASCII only
    return (ch >= '0') && (ch <= '9');
}

UINT32 getDigit(CHAR ch)
{
    // Process ASCII only
    return ((UINT32) ch - '0');
}
