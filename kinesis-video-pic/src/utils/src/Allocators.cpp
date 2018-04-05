#include "Include_i.h"

memAlloc globalMemAlloc = defaultMemAlloc;
memAlignAlloc globalMemAlignAlloc = defaultMemAlignAlloc;
memCalloc globalMemCalloc = defaultMemCalloc;
memFree globalMemFree = defaultMemFree;

VOID dumpMemoryHex(PVOID pMem, UINT32 size)
{
    DLOGS("============================================");
    DLOGS("Dumping memory: %p, size: %d", pMem, size);
    DLOGS("++++++++++++++++++++++++++++++++++++++++++++");

    CHAR buf[256];
    PCHAR pCur = buf;
    PBYTE pByte = (PBYTE) pMem;
    for(UINT32 i = 0; i < size; i++) {
        SPRINTF(pCur, "%02x ", *pByte++);
        pCur += 3;
        if ((i + 1) % 16 == 0) {
            DLOGS("%s", buf);
            buf[0] = 0;
            pCur = buf;
        }
    }

    DLOGS("++++++++++++++++++++++++++++++++++++++++++++");
    DLOGS("Dumping memory done!");
    DLOGS("============================================");
}