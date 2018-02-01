#ifndef TCP_CLIENT_H
#define TCP_CLIENT_H

/*
*********************************************************************************************************
*                                              INCLUDE FILES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                               CONSTANTS
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                             PERIPH DEFINES
*********************************************************************************************************
*/
#define     MAX_NAME_SIZE       32

#ifndef TCP_SERVER_H
struct name
{
        int     length;
        char    bytes[MAX_NAME_SIZE];
};
#endif

/*
*********************************************************************************************************
*                                               DATA TYPES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                            GLOBAL VARIABLES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                                 MACRO'S
*********************************************************************************************************
*/



/*
*********************************************************************************************************
*                                           FUNCTION PROTOTYPES
*********************************************************************************************************
*/

void Tcp_Client_Init(void);

/*
********************************************************************************************************
*                                             MODULE END
*********************************************************************************************************
*/

#endif /* TCP_CLIENT_H */
