#ifndef __ISO15765_4_H
#define __ISO15765_4_H
#include "obd.h"

void ISO15765_4_Config(OBD_PROTOCOL_E type);
OBD_PROTOCOL_E ISO15765_4_ProtocolDetect(ErrorStatus *err);
char* ISO15765_4_ReadDTC(ErrorStatus* err);
void ISO15765_4_CleanUpDTC(ErrorStatus* err);
char* ISO15765_4_ReadDS(OBD_DS_E cmd, ErrorStatus* err);

#endif


