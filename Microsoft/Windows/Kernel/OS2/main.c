/*
*********************************************************************************************************
*                                            EXAMPLE CODE
*
*               This file is provided as an example on how to use Micrium products.
*
*               Please feel free to use any application code labeled as 'EXAMPLE CODE' in
*               your application products.  Example code may be used as is, in whole or in
*               part, or may be used as a reference only. This file can be modified as
*               required to meet the end-product requirements.
*
*               Please help us continue to provide the Embedded community with the finest
*               software available.  Your honesty is greatly appreciated.
*
*               You can find our product's user manual, API reference, release notes and
*               more information at https://doc.micrium.com.
*               You can contact us at www.micrium.com.
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
*                                              uC/OS-II
*                                            EXAMPLE CODE
*
* Filename : main.c
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#include  <cpu.h>
#include  <lib_mem.h>
#include  <os.h>

#include  "app_cfg.h"


/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/

#pragma warning(disable:4996)
#define N 10


/*
*********************************************************************************************************
*                                       LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*/

static  OS_STK  StartupTaskStk[APP_CFG_STARTUP_TASK_STK_SIZE];
static  OS_STK  senderTaskStk[APP_CFG_STARTUP_TASK_STK_SIZE];
static  OS_STK  reciverTaskStk[APP_CFG_STARTUP_TASK_STK_SIZE];

static int stack[N];
static int head = 0;
static int tail = 0;
static INT16U sem = 0;
OS_EVENT* m;
OS_EVENT* empty;
OS_EVENT* full;


/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static  void  StartupTask (void  *p_arg);
static  void  sender(void* p_arg);

/*
*********************************************************************************************************
*                                                main()
*
* Description : This is the standard entry point for C code.  It is assumed that your code will call
*               main() once you have performed all necessary initialization.
*
* Arguments   : none
*
* Returns     : none
*
* Notes       : none
*********************************************************************************************************
*/



int  main (void)
{
#if OS_TASK_NAME_EN > 0u
    CPU_INT08U  os_err;
#endif


    CPU_IntInit();

    Mem_Init();                                                 /* Initialize Memory Managment Module                   */
    CPU_IntDis();                                               /* Disable all Interrupts                               */
    CPU_Init();                                                 /* Initialize the uC/CPU services                       */

    OSInit();                                                   /* Initialize uC/OS-II                                  */

    OSTaskCreateExt( StartupTask,                               /* Create the startup task                              */
                     0,
                    &StartupTaskStk[APP_CFG_STARTUP_TASK_STK_SIZE - 1u],
                     APP_CFG_STARTUP_TASK_PRIO,
                     APP_CFG_STARTUP_TASK_PRIO,
                    &StartupTaskStk[0u],
                     APP_CFG_STARTUP_TASK_STK_SIZE,
                     0u,
                    (OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR));



#if OS_TASK_NAME_EN > 0u
    OSTaskNameSet(         APP_CFG_STARTUP_TASK_PRIO,
                  (INT8U *)"Startup Task",
                           &os_err);
#endif
    OSStart();                                                  /* Start multitasking (i.e. give control to uC/OS-II)   */

    while (DEF_ON) {                                            /* Should Never Get Here.                               */
        ;
    }
}


/*
*********************************************************************************************************
*                                            STARTUP TASK
*
* Description : This is an example of a startup task.  As mentioned in the book's text, you MUST
*               initialize the ticker only once multitasking has started.
*
* Arguments   : p_arg   is the argument passed to 'StartupTask()' by 'OSTaskCreate()'.
*
* Returns     : none
*
* Notes       : 1) The first line of code is used to prevent a compiler warning because 'p_arg' is not
*                  used.  The compiler should not generate any code for this statement.
*********************************************************************************************************
*/

static int pop() {
    int output = stack[head];
    head++;
    //for circular buffer
    head = head % N;
    return output;
}

static void push(int input) {
    stack[tail] = input;
    tail++;
    //for circular buffer
    tail = tail % N;
}

static  void  sender(void* p_arg) {
    int input;
    INT8U err;
    while (DEF_TRUE) {
        //scanf input for stack push
        printf("sender@input >");
        scanf("%d", &input);
        //sem and mutex wait
        OSSemPend(empty, 0, &err);
        OSMutexPend(m, 0, &err);
        //stack push
        push(input);
        //sem and mutex signal
        OSMutexPost(m);
        OSSemPost(full);
        printf("sender@input: %d\n\r", input);
    }
}

static  void  reciver(void* p_arg) {
    int output;
    INT8U err;
    while (DEF_TRUE) {
        //delay 2sec
        OSTimeDlyHMSM(0u, 0u, 2u, 0u);
        //semapore and mutex wait
        OSSemPend(full, 0, &err);
        OSMutexPend(m, 0, &err);
        //stack pop
        output = pop();
        //sem and mutex signal
        OSMutexPost(m);
        OSSemPost(empty);
        
        printf("reciver@output: %d\n\r", output);
    }
}

static  void  StartupTask (void *p_arg)
{
   (void)p_arg;

    OS_TRACE_INIT();                                            /* Initialize the uC/OS-II Trace recorder               */

#if OS_CFG_STAT_TASK_EN > 0u
    OSStatTaskCPUUsageInit(&err);                               /* Compute CPU capacity with no task running            */
#endif

#ifdef CPU_CFG_INT_DIS_MEAS_EN
    CPU_IntDisMeasMaxCurReset();
#endif
    
    APP_TRACE_DBG(("uCOS-III is Running...\n\r"));
    INT8U os_err;
    m = OSMutexCreate(7u, &os_err);
    empty = OSSemCreate(N);
    full = OSSemCreate(0);

    //create sender and reciver task
    OSTaskCreate(sender, 0, &senderTaskStk[APP_CFG_STARTUP_TASK_STK_SIZE - 1u], 9u);
    OSTaskCreate(reciver, 0, &reciverTaskStk[APP_CFG_STARTUP_TASK_STK_SIZE - 1u], 8u);

    while (DEF_TRUE) {                                          /* Task body, always written as an infinite loop.       */
        OSTimeDlyHMSM(0u, 0u, 1u, 0u);
		APP_TRACE_DBG(("Time: %d\n\r", OSTimeGet()));
    }
}

