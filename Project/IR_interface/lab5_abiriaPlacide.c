// Serial C/ASM Mix Example
// Abiria Placide Lab5

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target Platform: EK-TM4C123GXL Evaluation Board
// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

// Hardware configuration:
// Red LED:
//   PF1 drives an NPN transistor that powers the red LED
// Green LED:
//   PF3 drives an NPN transistor that powers the green LED
// UART Interface:
//   U0TX (PA1) and U0RX (PA0) are connected to the 2nd controller
//   The USB on the 2nd controller enumerates to an ICDI interface and a virtual COM port
//   Configured to 115,200 baud, 8N1


//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------


#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "tm4c123gh6pm.h"
#include "uart0.h"


//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

// Blocking function that writes a serial character when the UART buffer is not full
extern void putcUart0(char c);

// Blocking function that writes a string when the UART buffer is not full
extern void putsUart0(char* str);

// Blocking function that returns with serial data once the buffer is not empty
extern char getcUart0();



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

char convertToLowerCase(char s)
{
    if(s >= 'A' && s <= 'Z')
    {
       s += 32;
    }
    return s;
}

void getsUart0(USER_DATA * data)
{
    uint32_t count = 0;
    //first clear buffer
    while(true)
    {
        //check number of chars available
        if (count == MAX_CHARS)        //if count == maxChars , add null terminator then return from function
        {
            data->buffer[count+1] = '\0';
            break;
        }

        //get char from UART0
        char c = getcUart0();

        //if char is  backspace ASCII 8 or 127 and count > 0, decrement count so last char is overwritten
        if (c == 8 || c == 127)
        {
            if (count > 0)
            {
                count--;
            }
        }
        //if char is carriage return, ASC   II 13, add null terminator ASCII 0, at end of string then return from function
        else if (c == 13)
        {
            data->buffer[count] = '\0';
            break; //or return data
        }
        //if char is  a space ASCII 32, or ascii >=32 then store char in data->buffer[count] then increment count.
        else if (c >= 32)
        {
            c = convertToLowerCase(c);
            data->buffer[count] = c;
            count++;
        }

        //loop back
    }
}

//parse fields

char *getFieldString(USER_DATA * data, uint8_t fieldNumber)
{

    if (fieldNumber > data->fieldCount)
    {
        return NULL;
    }

    else
    {
        char *field_ptr = &data->buffer[data->fieldPosition[fieldNumber]];
        return field_ptr;
    }

}

int32_t *getFieldInteger(USER_DATA* data, uint8_t fieldNumber)
{
	int32_t fieldvalue;
	int32_t * fieldvalue_ptr = &fieldvalue;

	if(fieldNumber > data->fieldCount)
	{
		return 0;
	}

	else
	{
			if(data->fieldType[fieldNumber] == 'n')
			{
				fieldvalue = data->buffer[data->fieldPosition[fieldNumber]] - 0x30; //convert to integer
				return fieldvalue_ptr;
			}

			else
			{
				return 0;
			}
	}
}

bool isCommand(USER_DATA* data, const char strCommand[], uint8_t minArguments)
{
	//find command
	uint32_t ret = strcmp(data->buffer, strCommand);
	if (ret == 0)
	{
		uint8_t args = data->fieldCount-1;
		if(args == minArguments)
		{
			return true;
		}

	    else
	    {
	        return false;
	    }
	}

	else
	{
	    return false;
	}
}



void parseFields(USER_DATA *data)
{

	//add \n to buffer
    //3 characters alpha: a-z A-z numeric: 0-9 delimiters: ~ , . :
    //comma = 44 ASCII
    //space = 32 ASCII

    data->fieldCount = 0;
    uint8_t position= 0;

	//this if statement is for the first assumption that the prev char at the beginning of the buffer is a  a delimiter
	//
    if (data->fieldCount == 0)
    {
        if ( data->buffer[position] >= 'a' &&  data->buffer[position] <= 'z' ) //alpha
        {
            data->fieldPosition[position] = position;
            data->fieldType[position] = 'a';
            data->fieldCount +=1;
        }

        else if ( (data->buffer[position]) >= '0' && (data->buffer[position] <= '9') ) //numeric
        {
            data->fieldPosition[position] = position;
            data->fieldType[position] = 'n';
            data->fieldCount +=1;
        }
    }

    int index = 0;

    for(index; data->buffer[index] != '\0'; index++)
    {
        if(data->fieldCount == MAX_FIELDS)
        {
			data->fieldType[MAX_FIELDS] = '\0';
            break;
        }

        else
        {
            char prev, next;
            prev = data->buffer[index];
            next = data->buffer[index+1];

            if (next != '\0') //what if its out of bounds
            {
                //check for transition from delimeter to alpha or numeric
                if ( !(prev >= 'a' && prev <= 'z' )) //out side of alpha alpha
                {
                    if ( !(prev >= '0' && prev <= '9')) //out side of numeric
                    {
						//check if transition occured from prev(delimeter) to next(alphanumeric)
						//data->buffer[index] = '\n';
                        if ( next >= 'a' &&  next <= 'z' ) //alpha

                        {
							data->buffer[index] = '\0';
                            data->fieldPosition[data->fieldCount] = index+1;
                            data->fieldType[data->fieldCount] = 'a';
                            data->fieldCount += 1;
                        }

                        else if ( next >= '0' && next <= '9') //numeric
                        {
							data->buffer[index] = '\0';
                            data->fieldPosition[data->fieldCount] = index+1;
                            data->fieldType[data->fieldCount] = 'n';
                            data->fieldCount +=1;
                        }

						else
						{
							data->buffer[index] = '\0';
						}
                    }

                }

            }

            //get previous char and next char, if prev is a delimeter (anything outside of alpha or numeric) and next is whithin a-z and 0-9 (aka 48-57 ASCII)
            //then  we have a transition. Record these values in the struct and update positions.
        }
    }
}


//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------

