/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/


#ifndef __OSALAPIH__
#define __OSALAPIH__

#include <stdint.h>

   /* Miscellaneous Type definitions that should already be defined,    */
   /* but are necessary.                                                */
#ifndef NULL
   #define NULL ((void *)0)
#endif

#ifndef TRUE
   #define TRUE (1 == 1)
#endif

#ifndef FALSE
   #define FALSE (0 == 1)
#endif

   /* Denotes the priority of the thread being created using the thread */
   /* create function (OSAL_CreateThread()).                            */
#define MINIMUM_THREAD_PRIORITY           (QURT_THREAD_MAX_PRIORITIES - 1)
#define DEFAULT_THREAD_PRIORITY           ((QURT_THREAD_MAX_PRIORITIES - 1) / 2)
#define MAXIMUM_THREAD_PRIORITY           0

   /* The following preprocessor definitions control the inclusion of   */
   /* debugging output.                                                 */
   /*                                                                   */
   /*    - DEBUG_ENABLED                                                */
   /*         - When defined enables debugging, if no other debugging   */
   /*           preprocessor definitions are defined then the debugging */
   /*           output is logged to a file (and included in the         */
   /*           driver).                                                */
   /*                                                                   */
   /*          - DEBUG_ZONES                                            */
   /*              - When defined (only when DEBUG_ENABLED is defined)  */
   /*                forces the value of this definition (unsigned long)*/
   /*                to be the Debug Zones that are enabled.            */
#define DBG_ZONE_CRITICAL_ERROR           (1 << 0)
#define DBG_ZONE_ENTER_EXIT               (1 << 1)
#define DBG_ZONE_OSAL                     (1 << 2)
#define DBG_ZONE_GENERAL                  (1 << 3)
#define DBG_ZONE_DEVELOPMENT              (1 << 4)
#define DBG_ZONE_VENDOR                   (1 << 5)

#define DBG_ZONE_ANY                      ((unsigned long)-1)

#ifndef DEBUG_ZONES
   #define DEBUG_ZONES                    DBG_ZONE_CRITICAL_ERROR
#endif

#ifndef MAX_DBG_DUMP_BYTES
   #define MAX_DBG_DUMP_BYTES             (((unsigned int)-1) - 1)
#endif

#ifdef DEBUG_ENABLED
   #define DBG_MSG(_zone_, _x_)           do { if(OSAL_TestDebugZone(_zone_)) OSAL_OutputMessage _x_; } while(0)
   #define DBG_DUMP(_zone_, _x_)          do { if(OSAL_TestDebugZone(_zone_)) OSAL_DumpData _x_; } while(0)
#else
   #define DBG_MSG(_zone_, _x_)
   #define DBG_DUMP(_zone_, _x_)
#endif

   /* The following constant defines a special length of time that      */
   /* specifies that there is to be NO Timeout waiting for some Event to*/
   /* occur (Mutexes, Semaphores, Events, etc).                         */
#define OSAL_INFINITE_WAIT                (0xFFFFFFFF)

#define OSAL_NO_WAIT                      0

   /* The following defines a Boolean_t type.                           */
typedef uint32_t Bool_t;

   /* The following type definition defines a BTPS Kernel API Event     */
   /* Handle.                                                           */
typedef void *Event_t;

   /* The following type definition defines a BTPS Kernel API Mutex     */
   /* Handle.                                                           */
typedef void *Mutex_t;

   /* The following type definition defines a BTPS Kernel API Thread    */
   /* Handle.                                                           */
typedef void *ThreadHandle_t;

   /* The following type definition defines a BTPS Kernel API Mailbox   */
   /* Handle.                                                           */
typedef void *Mailbox_t;

   /* The following MACRO is a utility MACRO that exists to calculate   */
   /* the offset position of a particular structure member from the     */
   /* start of the structure.  This MACRO accepts as the first          */
   /* parameter, the physical name of the structure (the type name, NOT */
   /* the variable name).  The second parameter to this MACRO represents*/
   /* the actual structure member that the offset is to be determined.  */
   /* This MACRO returns an unsigned integer that represents the offset */
   /* (in bytes) of the structure member.                               */
#define OSAL_STRUCTURE_OFFSET(_x, _y)     ((unsigned int)&(((_x *)0)->_y))

   /* The following type declaration represents the Prototype for a     */
   /* Thread Function.  This function represents the Thread that will be*/
   /* executed when passed to the OSAL_CreateThread() function.         */
   /* * NOTE * Once a Thread is created there is NO way to kill it.  The*/
   /*          Thread must exit by itself.                              */
typedef void *(*Thread_t)(void *ThreadParameter);

   /* The following function is responsible for delaying the current    */
   /* task for the specified duration (specified in Milliseconds).      */
   /* * NOTE * Very small timeouts might be smaller in granularity than */
   /*          the system can support !!!!                              */
void OSAL_Delay(unsigned long MilliSeconds);

   /* The following function is responsible for creating an actual Mutex*/
   /* (Binary Semaphore).  The Mutex is unique in that if a Thread      */
   /* already owns the Mutex, and it requests the Mutex again it will be*/
   /* granted the Mutex.  This is in Stark contrast to a Semaphore that */
   /* will block waiting for the second acquisition of the Sempahore.   */
   /* This function accepts as input whether or not the Mutex is        */
   /* initially Signalled or not.  If this input parameter is TRUE then */
   /* the caller owns the Mutex and any other threads waiting on the    */
   /* Mutex will block.  This function returns a NON-NULL Mutex Handle  */
   /* if the Mutex was successfully created, or a NULL Mutex Handle if  */
   /* the Mutex was NOT created.  If a Mutex is successfully created, it*/
   /* can only be destroyed by calling the OSAL_CloseMutex() function   */
   /* (and passing the returned Mutex Handle).                          */
Mutex_t OSAL_CreateMutex(Bool_t CreateOwned);

   /* The following function is responsible for waiting for the         */
   /* specified Mutex to become free.  This function accepts as input   */
   /* the Mutex Handle to wait for, and the Timeout (specified in       */
   /* Milliseconds) to wait for the Mutex to become available.  This    */
   /* function returns TRUE if the Mutex was successfully acquired and  */
   /* FALSE if either there was an error OR the Mutex was not acquired  */
   /* in the specified Timeout.  It should be noted that Mutexes have   */
   /* the special property that if the calling Thread already owns the  */
   /* Mutex and it requests access to the Mutex again (by calling this  */
   /* function and specifying the same Mutex Handle) then it will       */
   /* automatically be granted the Mutex.  Once a Mutex has been granted*/
   /* successfully (this function returns TRUE), then the caller MUST   */
   /* call the OSAL_ReleaseMutex() function.                            */
   /* * NOTE * There must exist a corresponding OSAL_ReleaseMutex()     */
   /*          function call for EVERY successful OSAL_WaitMutex()      */
   /*          function call or a deadlock will occur in the system !!! */
Bool_t OSAL_WaitMutex(Mutex_t Mutex, unsigned long Timeout);

   /* The following function is responsible for releasing a Mutex that  */
   /* was successfully acquired with the OSAL_WaitMutex() function.     */
   /* This function accepts as input the Mutex that is currently owned. */
   /* * NOTE * There must exist a corresponding OSAL_ReleaseMutex()     */
   /*          function call for EVERY successful OSAL_WaitMutex()      */
   /*          function call or a deadlock will occur in the system !!! */
void OSAL_ReleaseMutex(Mutex_t Mutex);

   /* The following function is responsible for destroying a Mutex that */
   /* was created successfully via a successful call to the             */
   /* OSAL_CreateMutex() function.  This function accepts as input the  */
   /* Mutex Handle of the Mutex to destroy.  Once this function is      */
   /* completed the Mutex Handle is NO longer valid and CANNOT be used. */
   /* Calling this function will cause all outstanding OSAL_WaitMutex() */
   /* functions to fail with an error.                                  */
void OSAL_CloseMutex(Mutex_t Mutex);

   /* The following function is responsible for creating an actual      */
   /* Event.  The Event is unique in that it only has two states.  These*/
   /* states are Signalled and Non-Signalled.  Functions are provided to*/
   /* allow the setting of the current state and to allow the option of */
   /* waiting for an Event to become Signalled.  This function accepts  */
   /* as input whether or not the Event is initially Signalled or not.  */
   /* If this input parameter is TRUE then the state of the Event is    */
   /* Signalled and any OSAL_WaitEvent() function calls will immediately*/
   /* return.  This function returns a NON-NULL Event Handle if the     */
   /* Event was successfully created, or a NULL Event Handle if the     */
   /* Event was NOT created.  If an Event is successfully created, it   */
   /* can only be destroyed by calling the OSAL_CloseEvent() function   */
   /* (and passing the returned Event Handle).                          */
Event_t OSAL_CreateEvent(Bool_t CreateSignalled);

   /* The following function is responsible for waiting for the         */
   /* specified Event to become Signalled.  This function accepts as    */
   /* input the Event Handle to wait for, and the Timeout (specified in */
   /* Milliseconds) to wait for the Event to become Signalled.  This    */
   /* function returns TRUE if the Event was set to the Signalled State */
   /* (in the Timeout specified) or FALSE if either there was an error  */
   /* OR the Event was not set to the Signalled State in the specified  */
   /* Timeout.  It should be noted that Signals have a special property */
   /* in that multiple Threads can be waiting for the Event to become   */
   /* Signalled and ALL calls to OSAL_WaitEvent() will return TRUE      */
   /* whenever the state of the Event becomes Signalled.                */
Bool_t OSAL_WaitEvent(Event_t Event, unsigned long Timeout);

   /* The following function is responsible for changing the state of   */
   /* the specified Event to the Non-Signalled State.  Once the Event is*/
   /* in this State, ALL calls to the OSAL_WaitEvent() function will    */
   /* block until the State of the Event is set to the Signalled State. */
   /* This function accepts as input the Event Handle of the Event to   */
   /* set to the Non-Signalled State.                                   */
void OSAL_ResetEvent(Event_t Event);

   /* The following function is responsible for changing the state of   */
   /* the specified Event to the Signalled State.  Once the Event is in */
   /* this State, ALL calls to the OSAL_WaitEvent() function will       */
   /* return.  This function accepts as input the Event Handle of the   */
   /* Event to set to the Signalled State.                              */
void OSAL_SetEvent(Event_t Event);

   /* The following function is responsible for changing the state of   */
   /* the specified Event to the Signalled State from an Interrupt.     */
   /* Once the Event is in this State, ALL calls to the OSAL_WaitEvent()*/
   /* function will return.  This function accepts as input the Event   */
   /* Handle of the Event to set to the Signalled State.                */
void OSAL_INT_SetEvent(Event_t Event);

   /* The following function is responsible for destroying an Event that*/
   /* was created successfully via a successful call to the             */
   /* OSAL_CreateEvent() function.  This function accepts as input the  */
   /* Event Handle of the Event to destroy.  Once this function is      */
   /* completed the Event Handle is NO longer valid and CANNOT be used. */
   /* Calling this function will cause all outstanding OSAL_WaitEvent() */
   /* functions to fail with an error.                                  */
void OSAL_CloseEvent(Event_t Event);

   /* The following function is responsible for creating an Event Group */
   /* with up to 32 useable bits.  Each bit within the event group can  */
   /* be set, cleared and waited upon.  Functions are provided to allow */
   /* the setting of the current states and to allow the option of      */
   /* waiting for an Event mask to become Signalled.  This function     */
   /* accepts as input which bits are initially Signalled.  This        */
   /* function returns a NON-NULL Event Handle if the Event was         */
   /* successfully created, or a NULL Event Handle if the Event was NOT */
   /* created.  If an Event is successfully created, it can only be     */
   /* destroyed by calling the OSAL_CloseEventGroup() function (and     */
   /* passing the returned Event Handle).                               */
Event_t OSAL_CreateEventGroup(uint32_t InitEventMask);

   /* The following function is responsible for waiting for one of the  */
   /* evetns in the specified Event Mask to become Signalled.  This     */
   /* function accepts as input the Event Handle to wait for, the Events*/
   /* that will cause the wait to unblock, whether the signalled events */
   /* should be cleared before return, and the Timeout (specified in    */
   /* Milliseconds) to wait for an event to become Signalled.  This     */
   /* function returns an event mask indicating which events have been  */
   /* signalled.  A value of zero is returned if the wait timed out or  */
   /* if an error occurred.                                             */
uint32_t OSAL_WaitEventGroup(Event_t Event, uint32_t EventMask, Bool_t AutoClear, unsigned long Timeout);

   /* The following function is responsible for clearing the state of   */
   /* the specified Event group.  The EventMask parameter is used to    */
   /* specify which events in the group are to be cleared.  Once the    */
   /* Event is in this State, ALL calls to the OSAL_WaitEventGroup()    */
   /* function will block until the State of the Event is set to the    */
   /* Signalled State.  This function accepts as input the Event Handle */
   /* of the Event to set to the Non-Signalled State.                   */
void OSAL_ResetEventGroup(Event_t Event, uint32_t EventMask);

   /* The following function is responsible for setting the state of the*/
   /* specified Event group.  The EventMask parameter is used to specify*/
   /* which events in the group are to be set.  Once the Event is in    */
   /* this State, ALL calls to the OSAL_WaitEventGroup() function will  */
   /* return.  This function accepts as input the Event Handle of the   */
   /* Event to set to the Signalled State.                              */
   /* * NOTE * This function is safe to call from ISR context.          */
void OSAL_SetEventGroup(Event_t Event, uint32_t EventMask);

   /* The following function is responsible for destroying an Event     */
   /* Group that was created successfully via a successful call to the  */
   /* OSAL_CreateEventGroup() function.  This function accepts as input */
   /* the Event Handle of the Event to destroy.  Once this function is  */
   /* completed the Event Handle is NO longer valid and CANNOT be used. */
   /* Calling this function will cause all outstanding                  */
   /* OSAL_WaitEventGroup() functions to fail with an error.            */
void OSAL_CloseEventGroup(Event_t Event);

   /* The following function is provided to allow a mechanism to        */
   /* actually allocate a Block of Memory (of at least the specified    */
   /* size).  This function accepts as input the size (in Bytes) of the */
   /* Block of Memory to be allocated.  This function returns a NON-NULL*/
   /* pointer to this Memory Buffer if the Memory was successfully      */
   /* allocated, or a NULL value if the memory could not be allocated.  */
void *OSAL_AllocateMemory(unsigned long MemorySize);

   /* The following function is responsible for de-allocating a Block of*/
   /* Memory that was successfully allocated with the                   */
   /* OSAL_AllocateMemory() function.  This function accepts a NON-NULL */
   /* Memory Pointer which was returned from the OSAL_AllocateMemory()  */
   /* function.  After this function completes the caller CANNOT use ANY*/
   /* of the Memory pointed to by the Memory Pointer.                   */
void OSAL_FreeMemory(void *MemoryPointer);

   /* The following function is responsible for copying a block of      */
   /* memory of the specified size from the specified source pointer to */
   /* the specified destination memory pointer.  This function accepts  */
   /* as input a pointer to the memory block that is to be Destination  */
   /* Buffer (first parameter), a pointer to memory block that points to*/
   /* the data to be copied into the destination buffer, and the size   */
   /* (in bytes) of the Data to copy.  The Source and Destination Memory*/
   /* Buffers must contain AT LEAST as many bytes as specified by the   */
   /* Size parameter.                                                   */
   /* * NOTE * This function does not allow the overlapping of the      */
   /*          Source and Destination Buffers !!!!                      */
void OSAL_MemCopy(void *Destination, const void *Source, unsigned long Size);

   /* The following function is responsible for moving a block of memory*/
   /* of the specified size from the specified source pointer to the    */
   /* specified destination memory pointer.  This function accepts as   */
   /* input a pointer to the memory block that is to be Destination     */
   /* Buffer (first parameter), a pointer to memory block that points to*/
   /* the data to be copied into the destination buffer, and the size   */
   /* (in bytes) of the Data to copy.  The Source and Destination Memory*/
   /* Buffers must contain AT LEAST as many bytes as specified by the   */
   /* Size parameter.                                                   */
   /* * NOTE * This function DOES allow the overlapping of the Source   */
   /*          and Destination Buffers.                                 */
void OSAL_MemMove(void *Destination, const void *Source, unsigned long Size);

   /* The following function is provided to allow a mechanism to fill a */
   /* block of memory with the specified value.  This function accepts  */
   /* as input a pointer to the Data Buffer (first parameter) that is to*/
   /* filled with the specified value (second parameter).  The final    */
   /* parameter to this function specifies the number of bytes that are */
   /* to be filled in the Data Buffer.  The Destination Buffer must     */
   /* point to a Buffer that is AT LEAST the size of the Size parameter.*/
void OSAL_MemInitialize(void *Destination, unsigned char Value, unsigned long Size);

   /* The following function is provided to allow a mechanism to Compare*/
   /* two blocks of memory to see if the two memory blocks (each of size*/
   /* Size (in bytes)) are equal (each and every byte up to Size bytes).*/
   /* This function returns a negative number if Source1 is less than   */
   /* Source2, zero if Source1 equals Source2, and a positive value if  */
   /* Source1 is greater than Source2.                                  */
int OSAL_MemCompare(const void *Source1, const void *Source2, unsigned long Size);

   /* The following function is provided to allow a mechanism to Compare*/
   /* two blocks of memory to see if the two memory blocks (each of size*/
   /* Size (in bytes)) are equal (each and every byte up to Size bytes) */
   /* using a Case-Insensitive Compare.  This function returns a        */
   /* negative number if Source1 is less than Source2, zero if Source1  */
   /* equals Source2, and a positive value if Source1 is greater than   */
   /* Source2.                                                          */
int OSAL_MemCompareI(const void *Source1, const void *Source2, unsigned long Size);

   /* The following function is provided to allow a mechanism to copy a */
   /* source NULL Terminated ASCII (character) String to the specified  */
   /* Destination String Buffer.  This function accepts as input a      */
   /* pointer to a buffer (Destination) that is to receive the NULL     */
   /* Terminated ASCII String pointed to by the Source parameter.  This */
   /* function copies the string byte by byte from the Source to the    */
   /* Destination (including the NULL terminator).                      */
void OSAL_StringCopy(char *Destination, const char *Source);

   /* The following function is provided to allow a mechanism to        */
   /* determine the Length (in characters) of the specified NULL        */
   /* Terminated ASCII (character) String.  This function accepts as    */
   /* input a pointer to a NULL Terminated ASCII String and returns the */
   /* number of characters present in the string (NOT including the     */
   /* terminating NULL character).                                      */
unsigned int OSAL_StringLength(const char *Source);

   /* The following function is provided to allow a mechanism for a C   */
   /* Run-Time Library () function implementation.  This function*/
   /* accepts as its imput the output buffer, a format string and a     */
   /* variable number of arguments determined by the format string.     */
int OSAL_SprintF(char *Buffer, const char *Format, ...);

   /* The following function is provided to allow a means for the       */
   /* programmer to create a separate thread of execution.  This        */
   /* function accepts as input the Function that represents the Thread */
   /* that is to be installed into the system as its first parameter.   */
   /* The second parameter is the size of the Threads Stack (in bytes)  */
   /* required by the Thread when it is executing.  The final parameter */
   /* to this function represents a parameter that is to be passed to   */
   /* the Thread when it is created.  This function returns a NON-NULL  */
   /* Thread Handle if the Thread was successfully created, or a NULL   */
   /* Thread Handle if the Thread was unable to be created.  Once the   */
   /* thread is created, the only way for the Thread to be removed from */
   /* the system is for the Thread function to run to completion.       */
   /* * NOTE * There does NOT exist a function to Kill a Thread that is */
   /*          present in the system.  Because of this, other means     */
   /*          needs to be devised in order to signal the Thread that it*/
   /*          is to terminate.                                         */
ThreadHandle_t OSAL_CreateThread(Thread_t ThreadFunction, unsigned int StackSize, void *ThreadParameter);

   /* The following function is provided to allow a means for the       */
   /* programmer to create a separate thread of execution.  This        */
   /* function accepts as input the Function that represents the Thread */
   /* that is to be installed into the system as its first parameter.   */
   /* The second parameter is the size of the Threads Stack (in bytes)  */
   /* required by the Thread when it is executing.  The third parameter */
   /* to this function represents a parameter that is to be passed to   */
   /* the Thread when it is created.  The final parameter is the        */
   /* priority of the thread.  This function returns a NON-NULL Thread  */
   /* Handle if the Thread was successfully created, or a NULL Thread   */
   /* Handle if the Thread was unable to be created.  Once the thread is*/
   /* created, the only way for the Thread to be removed from the system*/
   /* is for the Thread function to run to completion.                  */
   /* * NOTE * There does NOT exist a function to Kill a Thread that is */
   /*          present in the system.  Because of this, other means     */
   /*          needs to be devised in order to signal the Thread that it*/
   /*          is to terminate.                                         */
ThreadHandle_t OSAL_CreateThreadPriority(Thread_t ThreadFunction, unsigned int StackSize, void *ThreadParameter, unsigned int Priority);

   /* The following function is provided to allow a mechanism to        */
   /* retrieve the handle of the thread which is currently executing.   */
   /* This function require no input parameters and will return a valid */
   /* ThreadHandle upon success.                                        */
ThreadHandle_t OSAL_CurrentThreadHandle(void);

   /* The following function attempts to set the priority on a thread   */
   /* created by OSAL_CreateThread() or OSAL_CreateThreadPriority(). It */
   /* returns TRUE upon successfully setting the priority, otherwise    */
   /* it returns FALSE.                                                 */
Bool_t OSAL_SetThreadPriority(ThreadHandle_t ThreadHandle, unsigned int Priority);

   /* The following function is provided to allow a mechanism to enable */
   /* the scheduler.                                                    */
void OSAL_EnableScheduler(void);

   /* The following function is provided to allow a mechanism to disable*/
   /* the scheduler.                                                    */
void OSAL_DisableScheduler(void);

   /* The following function is provided to allow a mechanism to create */
   /* a Mailbox.  A Mailbox is a Data Store that contains slots (all of */
   /* the same size) that can have data placed into (and retrieved      */
   /* from).  Once Data is placed into a Mailbox (via the               */
   /* OSAL_AddMailbox() function, it can be retrieved by using the      */
   /* OSAL_WaitMailbox() function.  Data placed into the Mailbox is     */
   /* retrieved in a FIFO method.  This function accepts as input the   */
   /* Maximum Number of Slots that will be present in the Mailbox and   */
   /* the Size of each of the Slots.  This function returns a NON-NULL  */
   /* Mailbox Handle if the Mailbox is successfully created, or a NULL  */
   /* Mailbox Handle if the Mailbox was unable to be created.           */
Mailbox_t OSAL_CreateMailbox(unsigned int NumberSlots, unsigned int SlotSize);

   /* The following function is provided to allow a means to Add data to*/
   /* the Mailbox (where it can be retrieved via the OSAL_WaitMailbox() */
   /* function.  This function accepts as input the Mailbox Handle of   */
   /* the Mailbox to place the data into and a pointer to a buffer that */
   /* contains the data to be added.  This pointer *MUST* point to a    */
   /* data buffer that is AT LEAST the Size of the Slots in the Mailbox */
   /* (specified when the Mailbox was created) and this pointer CANNOT  */
   /* be NULL.  The data that the MailboxData pointer points to is      */
   /* placed into the Mailbox where it can be retrieved via the         */
   /* OSAL_WaitMailbox() function.                                      */
   /* * NOTE * This function copies from the MailboxData Pointer the    */
   /*          first SlotSize Bytes.  The SlotSize was specified when   */
   /*          the Mailbox was created via a successful call to the     */
   /*          OSAL_CreateMailbox() function.                           */
Bool_t OSAL_AddMailbox(Mailbox_t Mailbox, void *MailboxData, unsigned long Timeout);

   /* The following function is provided to allow a means to retrieve   */
   /* data from the specified Mailbox.  This function will block until  */
   /* either Data is placed in the Mailbox or an error with the Mailbox */
   /* was detected.  This function accepts as its first parameter a     */
   /* Mailbox Handle that represents the Mailbox to wait for the data   */
   /* with.  This function accepts as its second parameter, a pointer to*/
   /* a data buffer that is AT LEAST the size of a single Slot of the   */
   /* Mailbox (specified when the OSAL_CreateMailbox() function was     */
   /* called).  The MailboxData parameter CANNOT be NULL.  This function*/
   /* will return TRUE if data was successfully retrieved from the      */
   /* Mailbox or FALSE if there was an error retrieving data from the   */
   /* Mailbox.  If this function returns TRUE then the first SlotSize   */
   /* bytes of the MailboxData pointer will contain the data that was   */
   /* retrieved from the Mailbox.                                       */
   /* * NOTE * This function copies to the MailboxData Pointer the data */
   /*          that is present in the Mailbox Slot (of size SlotSize).  */
   /*          The SlotSize was specified when the Mailbox was created  */
   /*          via a successful call to the OSAL_CreateMailbox()        */
   /*          function.                                                */
Bool_t OSAL_WaitMailbox(Mailbox_t Mailbox, void *MailboxData, unsigned long Timeout);

   /* The following function is responsible for destroying a Mailbox    */
   /* that was created successfully via a successful call to the        */
   /* OSAL_CreateMailbox() function.  This function accepts as input the*/
   /* Mailbox Handle of the Mailbox to destroy.  Once this function is  */
   /* completed the Mailbox Handle is NO longer valid and CANNOT be     */
   /* used.  Calling this function will cause all outstanding           */
   /* OSAL_WaitMailbox() functions to fail with an error.               */
void OSAL_DeleteMailbox(Mailbox_t Mailbox);

   /* The following function gets the current system ticks.             */
unsigned long OSAL_Get_System_Ticks(void);

   /* The following function converts system ticks to milliseconds.     */
unsigned long OSAL_Ticks_To_Milliseconds(unsigned long Ticks);

   /* The following function is used to initialize the Platform module. */
   /* The Platform module relies on some static variables that are used */
   /* to coordinate the abstraction.  When the module is initially      */
   /* started from a cold boot, all variables are set to the proper     */
   /* state.  If the Warm Boot is required, then these variables need to*/
   /* be reset to their default values.  This function sets all static  */
   /* parameters to their default values.                               */
   /* * NOTE * The implementation is free to pass whatever information  */
   /*          required in this parameter.                              */
void OSAL_Init(void *UserParam);

   /* The following function is used to cleanup the Platform module.    */
void OSAL_DeInit(void);

   /* Write out the specified NULL terminated Debugging String to the   */
   /* Debug output.                                                     */
void OSAL_OutputMessage(const char *DebugString, ...);

   /* The following function is used to set the Debug Mask that controls*/
   /* which debug zone messages get displayed.  The function takes as   */
   /* its only parameter the Debug Mask value that is to be used.  Each */
   /* bit in the mask corresponds to a debug zone.  When a bit is set,  */
   /* the printing of that debug zone is enabled.                       */
void OSAL_SetDebugMask(unsigned long DebugMask);

   /* The following function is a utility function that can be used to  */
   /* determine if a specified Zone is currently enabled for debugging. */
int OSAL_TestDebugZone(unsigned long Zone);

   /* The following function is responsible for displaying binary debug */
   /* data.  The first parameter to this function is the length of data */
   /* pointed to by the next parameter.  The final parameter is a       */
   /* pointer to the binary data to be displayed.                       */
int OSAL_DumpData(unsigned int DataLength, const unsigned char *DataPtr);

#endif
