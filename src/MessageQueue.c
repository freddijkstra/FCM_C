// -------------------------------------------------------------------------------------------------
// MessageQueue.c
// -------------------------------------------------------------------------------------------------

#include "MessageQueue.h"


#include <string.h> // For memcpy

// -------------------------------------------------------------------------------------------------


// -------------------------------------------------------------------------------------------------
// MessageQueue_init
// -------------------------------------------------------------------------------------------------
// Initialize the message queue administration.
// -------------------------------------------------------------------------------------------------
// - pMsgQueue : pointer to the created message queue administration.
// - queueSize : the size of the message queue expressed in number of data-less messages.
// -------------------------------------------------------------------------------------------------
void MessageQueue_init(MessageQueue_t* pMsgQueue, int queueSize )
{
  pMsgQueue->pWrite = pMsgQueue->pMessage;
  pMsgQueue->pRead = pMsgQueue->pWrite;
  pMsgQueue->pWrapAround = &pMsgQueue->pMessage[queueSize-1]+1;
  pMsgQueue->pEnd_of_queue = &pMsgQueue->pMessage[queueSize-1]+1;
}


// -------------------------------------------------------------------------------------------------
// PrepareMessage
// -------------------------------------------------------------------------------------------------
// Make room for the new message and get the location in the new message in the message queue
// and set the fields already.
// -------------------------------------------------------------------------------------------------
// - pMsgQueue : pointer to the message queue.
// - pMsgId : message id, implemented as a pointer to a literal string.
// - msgSize : size of the message.
// -------------------------------------------------------------------------------------------------
void* PrepareMessage(MessageQueue_t* pMsgQueue, char* pMsgId, int msgSize)
{

	// Wrap around when there is not enough room.
	if( sizeof(Message_t)+msgSize-sizeof(char*) >
		(address_t)pMsgQueue->pEnd_of_queue-(address_t)pMsgQueue->pWrite)
	{
		// Note that when the message is not sent in the transition,
		// this wrap around is done for nothing.
		pMsgQueue->pWrapAround = pMsgQueue->pWrite;
		pMsgQueue->pWrite = pMsgQueue->pMessage;

		// Read pointer
		if( pMsgQueue->pRead == pMsgQueue->pWrapAround )
		  pMsgQueue->pRead = pMsgQueue->pMessage;

	}
	pMsgQueue->pWrite->pMsgId = pMsgId;
	pMsgQueue->pWrite->msgSize = msgSize;
	return &pMsgQueue->pWrite->pMsgId;
}

// -------------------------------------------------------------------------------------------------
// ShiftWritePointer
// -------------------------------------------------------------------------------------------------
// Shift the write-pointer.
// -------------------------------------------------------------------------------------------------
// - pMsgQueue : pointer to the message queue.
// -------------------------------------------------------------------------------------------------
void ShiftWritePointer(MessageQueue_t* pMsgQueue)
{
	pMsgQueue->pWrite = (void *)((address_t)&pMsgQueue->pWrite->pMsgId + pMsgQueue->pWrite->msgSize);
	if( (void *)pMsgQueue->pWrite > (void *)pMsgQueue->pWrapAround )
		// 'Reset' the wrap-around pointer.
		pMsgQueue->pWrapAround = pMsgQueue->pEnd_of_queue;
	// If the write pointer is now pointing at the end of the queue.
	if( pMsgQueue->pWrite == pMsgQueue->pEnd_of_queue )
		pMsgQueue->pWrite = pMsgQueue->pMessage;
}

// -------------------------------------------------------------------------------------------------
// SendMessage
// -------------------------------------------------------------------------------------------------
// Set the last fields and shift the write-pointer.
// -------------------------------------------------------------------------------------------------
// - pMsgQueue : pointer to the message queue.
// - pInterface : pointer to the interface on which the message is to be sent.
// -------------------------------------------------------------------------------------------------
void SendMessage(MessageQueue_t* pMsgQueue, Interface_t* pInterface )
{
	pMsgQueue->pWrite->systemTime = GetSystemTime();
	pMsgQueue->pWrite->pInterface = pInterface->pRemoteInterface;

	ShiftWritePointer( pMsgQueue );
}

// -------------------------------------------------------------------------------------------------
// CopyMessages
// -------------------------------------------------------------------------------------------------
// Copy all the messages from one message queue into the other.
// -------------------------------------------------------------------------------------------------
// - pSubMsgQueue : pointer to the message queue from which the message must be copied.
// - pMainMsgQueue : pointer to the message queue to which the message must be copied.
// -------------------------------------------------------------------------------------------------
void CopyMessages(MessageQueue_t* pSubMsgQueue, MessageQueue_t* pMainMsgQueue)
{
	// Check whether there is a message to copy.
	while( pSubMsgQueue->pRead != pSubMsgQueue->pWrite )
	{
		if( pSubMsgQueue->pRead->pInterface != NULL ) // Was message deleted?
		{
			// -- Copy message --

			// Create room in destination message queue.
			PrepareMessage(pMainMsgQueue, pSubMsgQueue->pRead->pMsgId, pSubMsgQueue->pRead->msgSize);

			// Copy the structure
			*pMainMsgQueue->pWrite = *pSubMsgQueue->pRead;

			// Copy the message parameters (includes message id).
			memcpy( &pMainMsgQueue->pWrite->pMsgId, &pSubMsgQueue->pRead->pMsgId, pSubMsgQueue->pRead->msgSize);

			// Now 'send' the message by shifting the write-pointer.
			ShiftWritePointer(pMainMsgQueue);
		}

		// Message from source message-queue 'handled'.
		NEXT_MESSAGE(pSubMsgQueue);
	}
}


