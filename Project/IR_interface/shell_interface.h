#ifndef LAB5_ABIRIAPLACIDE_H_
#define LAB5_ABIRIAPLACIDE_H_


//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------
//UI Information
#define MAX_CHARS 80
#define MAX_FIELDS 5

typedef struct USER_DATA
{
    char buffer[MAX_CHARS+1];
    uint8_t fieldCount;
    uint8_t fieldPosition[MAX_FIELDS];
    char fieldType[MAX_FIELDS];
}USER_DATA;

void initLab5();
void getsUart0(USER_DATA * data);
void parseFields(USER_DATA *data);
char *getFieldString(USER_DATA * data, uint8_t fieldNumber);
int32_t *getFieldInteger(USER_DATA* data, uint8_t fieldNumber);
bool isCommand(USER_DATA* data, const char strCommand[], uint8_t minArguments);

#endif
