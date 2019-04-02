/*
 * Copyright (c) 2001-2003 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *
 */

/* lwIP includes. */
#include "lwip/debug.h"
#include "lwip/def.h"
#include "lwip/sys.h"
#include "lwip/mem.h"
#include "lwip/stats.h"
#include "FreeRTOS.h"
#include "task.h"

#include "bsp.h"

/**
 * @ingroup sys_time
 * Returns the current time in milliseconds,
 * may be the same as sys_jiffies or at least based on it.
 */

u32_t sys_now(void)
{
	return xTaskGetTickCount();
}

#ifndef sys_jiffies

/**
 * Ticks/jiffies since power up.
 */
u32_t sys_jiffies(void)
{
	return xTaskGetTickCount();
}

#endif /* #ifndef sys_jiffies */


/**
 * @ingroup sys_mbox
 * Create a new mbox of specified size
 * @param mbox pointer to the mbox to create
 * @param size (minimum) number of messages in this mbox，不能大于archMAX_MESG_QUEUE_LENGTH=6
 * @return ERR_OK if successful, another err_t otherwise
 */
err_t sys_mbox_new(sys_mbox_t *mbox, int size)
{
	
	UBaseType_t archMessageLength = size;
	
	if( size > archMAX_MESG_QUEUE_LENGTH )
		archMessageLength = archMAX_MESG_QUEUE_LENGTH ;
	
	*mbox = xQueueCreate( archMessageLength, sizeof( void * ) );

	if (*mbox == NULL)
		return ERR_MEM;
 
	return ERR_OK;
 
}

/**
 * @ingroup sys_mbox
 * Delete an mbox
 * @param mbox mbox to delete
 */
void sys_mbox_free(sys_mbox_t *mbox)
{
	
	UBaseType_t uxReturn;
	
	if( *mbox == NULL )
		return;
	
	uxReturn = uxQueueMessagesWaiting( *mbox );
	
	if( uxReturn )
	{
		/* Line for breakpoint.  Should never break here! */
			
		// TODO notify the user of failure.
		LWIP_ASSERT( "sys_mbox_free err uxQueueMessagesWaiting.\r\n ", uxReturn == pdTRUE );
	}

	vQueueDelete( *mbox );

}

/**
 * @ingroup sys_mbox
 * Post a message to an mbox - may not fail
 * -> blocks if full, only used from tasks not from ISR
 * @param mbox mbox to posts the message
 * @param msg message to post (ATTENTION: can be NULL)
 */
void sys_mbox_post(sys_mbox_t *mbox, void *data)
{
	if( *mbox == NULL )
		return;
	
	xQueueSendToBack(*mbox, &data, portMAX_DELAY );
	
}

/**
 * @ingroup sys_mbox
 * Try to post a message to an mbox - may fail if full or ISR
 * @param mbox mbox to posts the message
 * @param msg message to post (ATTENTION: can be NULL)
 */
err_t sys_mbox_trypost(sys_mbox_t *mbox, void *msg)
{
	 err_t result;

	 if( *mbox == NULL )
			return ERR_MEM ;

   if ( xQueueSend( *mbox, &msg, 0 ) == pdTRUE )
   {
      result = ERR_OK;
   }
   else 
	 {
      // could not post, queue must be full
      result = ERR_MEM;
		 
   }

   return result;
}

/**
 * @ingroup sys_mbox
 * Try to post a message to an mbox - may fail if full or ISR
 * @param mbox mbox to posts the message
 * @param msg message to post (ATTENTION: can be NULL)
 */
err_t sys_mbox_trypost_fromisr(sys_mbox_t *mbox, void *msg)
{
	 err_t result;
	 BaseType_t xHigherPriorityTaskWoken;
	
	 if( *mbox == NULL )
			return ERR_MEM ;

   if ( xQueueSendFromISR( *mbox, &msg, &xHigherPriorityTaskWoken ) == pdTRUE )
   {
      result = ERR_OK;
   }
   else 
	 {
      // could not post, queue must be full
      result = ERR_MEM;
		 
   }

		// Actual macro used here is port specific.
		portYIELD_FROM_ISR( xHigherPriorityTaskWoken );

   return result;
}

/**
 * @ingroup sys_mbox
 * Wait for a new message to arrive in the mbox
 * @param mbox mbox to get a message from
 * @param msg pointer where the message is stored
 * @param timeout maximum time (in milliseconds) to wait for a message (0 = wait forever)
 * @return time (in milliseconds) waited for a message, may be 0 if not waited
           or SYS_ARCH_TIMEOUT on timeout
 *         The returned time has to be accurate to prevent timer jitter!
 */
u32_t sys_arch_mbox_fetch(sys_mbox_t *mbox, void **msg, u32_t timeout)
{
	void *dummyptr;
	
	BaseType_t err;
	
	portTickType StartTime, EndTime, Elapsed;

	StartTime = xTaskGetTickCount();
	
	if( *mbox == NULL )
		return SYS_ARCH_TIMEOUT;

	if ( msg == NULL )
	{
		msg = &dummyptr;
	}
		
	if ( timeout != 0 )
	{
		if ( pdTRUE == xQueueReceive( *mbox, &(*msg), timeout / portTICK_RATE_MS ) )
		{
			EndTime = xTaskGetTickCount();
			Elapsed = (EndTime - StartTime) * portTICK_RATE_MS;
			
			return ( Elapsed );
		}
		else // timed out blocking for message
		{
			*msg = NULL;
			
			return SYS_ARCH_TIMEOUT;
		}
	}
	else // block forever for a message.
	{
		err = xQueueReceive( *mbox, &(*msg), portMAX_DELAY );
		
		if( pdTRUE != err )
		{
			LWIP_ASSERT("sys_arch_mbox_fetch xQueueReceive returned with error!", err == pdTRUE);
		}
		
		EndTime = xTaskGetTickCount();
		Elapsed = (EndTime - StartTime) * portTICK_RATE_MS;
		
		return ( Elapsed ); // return time blocked TODO test	
	}
}

#ifndef sys_arch_mbox_tryfetch

/**
 * @ingroup sys_mbox
 * Wait for a new message to arrive in the mbox
 * @param mbox mbox to get a message from
 * @param msg pointer where the message is stored
 * @return 0 (milliseconds) if a message has been received
 *         or SYS_MBOX_EMPTY if the mailbox is empty
 */
u32_t sys_arch_mbox_tryfetch(sys_mbox_t *mbox, void **msg)
{
	void *dummyptr;
	
	if( *mbox == NULL )
		return SYS_ARCH_TIMEOUT;

	if ( msg == NULL )
	{
		msg = &dummyptr;
	}

   if ( pdTRUE == xQueueReceive( *mbox, &(*msg), 0 ) )
   {
      return ERR_OK;
   }
   else
   {
      return SYS_MBOX_EMPTY;
   }
}

#endif /* #ifndef sys_arch_mbox_tryfetch */

#ifndef sys_mbox_valid

/**
 * @ingroup sys_mbox
 * Check if an mbox is valid/allocated: return 1 for valid, 0 for invalid
 */
int sys_mbox_valid(sys_mbox_t *mbox)          
{      
  if (*mbox == SYS_MBOX_NULL) 
    return 0;
  else
    return 1;
}                

#endif /* #ifndef sys_mbox_valid */

#ifndef sys_mbox_set_invalid

/**
 * @ingroup sys_mbox
 * Set an mbox invalid so that sys_mbox_valid returns 0
 */                                           
void sys_mbox_set_invalid(sys_mbox_t *mbox)   
{                                             
  *mbox = SYS_MBOX_NULL;                      
}
#endif /* #ifndef sys_mbox_set_invalid */

/**
 * @ingroup sys_sem
 * Create a new semaphore
 * @param sem pointer to the semaphore to create
 * @param count initial count of the semaphore
 * @return ERR_OK if successful, another err_t otherwise
 */
err_t sys_sem_new( sys_sem_t *sem, u8_t count )
{
	
	vSemaphoreCreateBinary( *sem );
	
	if(*sem == NULL)
	{
		return ERR_MEM;
	}
	
	if(count == 0)	// Means it can't be taken
	{
		xSemaphoreTake(*sem,1);
	}
	
	return ERR_OK;
	
}

/**
 * @ingroup sys_sem
 * Wait for a semaphore for the specified timeout
 * @param sem the semaphore to wait for
 * @param timeout timeout in milliseconds to wait (0 = wait forever)
 * @return time (in milliseconds) waited for the semaphore
 *         or SYS_ARCH_TIMEOUT on timeout
 */
u32_t sys_arch_sem_wait(sys_sem_t *sem, u32_t timeout)
{
	portTickType StartTime, EndTime, Elapsed;
	
	BaseType_t err;

	StartTime = xTaskGetTickCount();
	
	if( *sem == NULL )
		return SYS_ARCH_TIMEOUT;

	if(	timeout != 0)
	{
		if( xSemaphoreTake( *sem, timeout / portTICK_RATE_MS ) == pdTRUE )
		{
			EndTime = xTaskGetTickCount();
			Elapsed = (EndTime - StartTime) * portTICK_RATE_MS;
			
			return (Elapsed); // return time blocked TODO test	
		}
		else
		{
			return SYS_ARCH_TIMEOUT;
		}
	}
	else // must block without a timeout
	{
		
		err = xSemaphoreTake(*sem, portMAX_DELAY);
		
		if( pdTRUE != err )
		{
			LWIP_ASSERT("sys_arch_mbox_fetch xQueueReceive returned with error!", err == pdTRUE);
		}
		
		EndTime = xTaskGetTickCount();
		Elapsed = (EndTime - StartTime) * portTICK_RATE_MS;

		return ( Elapsed ); // return time blocked	
		
	}
}


/**
 * @ingroup sys_sem
 * Signals a semaphore
 * @param sem the semaphore to signal
 */
void sys_sem_signal(sys_sem_t *sem)
{
	
	if( *sem == NULL )
		return ;
	
	xSemaphoreGive(*sem);
	
}

/**
 * @ingroup sys_sem
 * Delete a semaphore
 * @param sem semaphore to delete
 */
void sys_sem_free(sys_sem_t *sem)
{
	vSemaphoreDelete(*sem);
}

#ifndef sys_sem_valid

/**
 * @ingroup sys_sem
 * Check if a semaphore is valid/allocated: return 1 for valid, 0 for invalid
 */
int sys_sem_valid(sys_sem_t *sem)                                               
{
  if (*sem == SYS_SEM_NULL)
    return 0;
  else
    return 1;                                       
}

#endif /* #ifndef sys_sem_valid */

#ifndef sys_sem_set_invalid

/**
 * @ingroup sys_sem
 * Set a semaphore invalid so that sys_sem_valid returns 0
 */                                                                                                                                                        
void sys_sem_set_invalid(sys_sem_t *sem)                                        
{                                                                               
  *sem = SYS_SEM_NULL;                                                          
} 

#endif /* #ifndef sys_sem_set_invalid */

/* sys_init() must be called before anything else. */
void sys_init(void)
{
    /* nothing on FreeRTOS porting */
}


#if LWIP_COMPAT_MUTEX == 0

/**
 * @ingroup sys_mutex
 * Create a new mutex.
 * Note that mutexes are expected to not be taken recursively by the lwIP code,
 * so both implementation types (recursive or non-recursive) should work.
 * @param mutex pointer to the mutex to create
 * @return ERR_OK if successful, another err_t otherwise
 */
err_t sys_mutex_new(sys_mutex_t *mutex) 
{

  *mutex = xSemaphoreCreateMutex();
	
	if(*mutex == NULL)
	{
		return ERR_MEM;
	}
	
  return ERR_OK;
	
}

/**
 * @ingroup sys_mutex
 * Delete a semaphore
 * @param mutex the mutex to delete
 */
void sys_mutex_free(sys_mutex_t *mutex)
{
	vSemaphoreDelete(*mutex);
}

/**
 * @ingroup sys_mutex
 * Lock a mutex
 * @param mutex the mutex to lock
 */
void sys_mutex_lock(sys_mutex_t *mutex)
{
	if( *mutex == NULL )
			return ;
		
	sys_arch_sem_wait(mutex, 0);
}

/**
 * @ingroup sys_mutex
 * Unlock a mutex
 * @param mutex the mutex to unlock
 */
void sys_mutex_unlock(sys_mutex_t *mutex)
{
	
	if( *mutex == NULL )
			return ;
		
	xSemaphoreGive(*mutex);
}

#endif /*LWIP_COMPAT_MUTEX*/


/**
 * @ingroup sys_misc
 * The only thread function:
 * Creates a new thread
 * ATTENTION: although this function returns a value, it MUST NOT FAIL (ports have to assert this!)
 * @param name human-readable name for the thread (used for debugging purposes)
 * @param thread thread-function
 * @param arg parameter passed to 'thread'
 * @param stacksize stack size in bytes for the new thread (may be ignored by ports)
 * @param prio priority of the new thread (may be ignored by ports) */
sys_thread_t sys_thread_new(const char *name, lwip_thread_fn thread , void *arg, int stacksize, int prio)
{
	sys_thread_t createdTaskHandle = NULL ;
	
	int result;

  result = xTaskCreate( thread, name, stacksize, arg, prio, &createdTaskHandle );
	
	if(result == pdPASS)
	{
	 return createdTaskHandle;
	}
	else
	{
	 return NULL;
	}
}


/**
 * @ingroup sys_prot
 * SYS_ARCH_PROTECT
 * Perform a "fast" protect. This could be implemented by
 * disabling interrupts for an embedded system or by using a semaphore or
 * mutex. The implementation should allow calling SYS_ARCH_PROTECT when
 * already protected. The old protection level is returned in the variable
 * "lev". This macro will default to calling the sys_arch_protect() function
 * which should be implemented in sys_arch.c. If a particular port needs a
 * different implementation, then this macro may be defined in sys_arch.h
 */
sys_prot_t sys_arch_protect(void)
{
	vPortEnterCritical();
	return 1;
}

/**
 * @ingroup sys_prot
 * SYS_ARCH_UNPROTECT
 * Perform a "fast" set of the protection level to "lev". This could be
 * implemented by setting the interrupt level to "lev" within the MACRO or by
 * using a semaphore or mutex.  This macro will default to calling the
 * sys_arch_unprotect() function which should be implemented in
 * sys_arch.c. If a particular port needs a different implementation, then
 * this macro may be defined in sys_arch.h
 */
void sys_arch_unprotect(sys_prot_t pval)
{
	( void ) pval;
	vPortExitCritical();
}


void sys_arch_assert(const char *file, int line)
{

	printf( "sys_arch_assert: %d in %s, pcTaskGetTaskName:%s.r\n", line, file, pcTaskGetTaskName( NULL ) );
	//while(1);

}
