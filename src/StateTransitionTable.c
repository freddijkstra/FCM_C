// -------------------------------------------------------------------------------------------------
// StateTransition.c
// -------------------------------------------------------------------------------------------------

#include <assert.h>
#include "StateTransitionTable.h"


// -------------------------------------------------------------------------------------------------
void SetSttElements(SttElement_t* pTable, int numElements)
{
	int i;
	for(i=0;i<numElements;i++)
	{
		pTable[i].pNextElement = NULL;
		pTable[i].pReference = NULL;
	}
}


// -------------------------------------------------------------------------------------------------
SttElement_t* GetEmptyElement(SttElement_t* pTable, int bufferSize, int numElements)
{
	SttElement_t* pElement = NULL;

	int i;
	for( i=0; i<numElements; i++)
	{
		if( pTable[i].pNextElement == NULL && pTable[i].pReference == NULL)
		{
			pElement = &pTable[i];
			break;
		}
	}

	assert(pElement != NULL && numElements-i >= bufferSize);
	return pElement;
}



// -------------------------------------------------------------------------------------------------
// CreateTransition
// -------------------------------------------------------------------------------------------------
// Create a new transition.
// Returns the pointer to the first SttElement_t of the created transition.
// -------------------------------------------------------------------------------------------------
// - level : whether the first element is a state, interface or message.
// - pTable : pointer to the table of SttElement_t elements.
// - numElements : number of elements in the pTable.
// - pState: pointer to the state of the transition.
// - pInterface: pointer to the interface of the transition.
// - pMsg: pointer to the message of the transition.
// - pTransFunc: pointer to the transition-function of the transition.
// - pNextState: pointer to the next state of the transition.
// -------------------------------------------------------------------------------------------------

SttElement_t* CreateTransition
(
		SttReferenceType_t level,
		SttElement_t* pTable,
		int   numElements,
		void* pState,
		void* pInterface,
		void* pMsg,
		void *pTransFunc,
		void* pNextState
)
{
	int numEntries = TRANSITION_SIZE-(int)level;

	// Create the necessary elements for the new transition.
	SttElement_t* newSttEntry = GetEmptyElement(pTable, numEntries, numElements);

	// -- Set the reference depending on what must be set --

	if( level == Stt_State ) 		newSttEntry[Stt_State].pReference = pState;
	if( level <= Stt_Interface ) 	newSttEntry[Stt_Interface-level].pReference = pInterface;
	if( level <= Stt_Message ) 		newSttEntry[Stt_Message-level].pReference = pMsg;
									newSttEntry[Stt_TransitionFunction-level].pReference = pTransFunc;

	newSttEntry[Stt_TransitionFunction-level].pNextElement = pNextState; // Temp.

	return newSttEntry;
}


// -------------------------------------------------------------------------------------------------
// Transition
// -------------------------------------------------------------------------------------------------
// Add the given transition to the STT.
// Returns the pointer to the first transition. This is done to be able to allocate the first.
// -------------------------------------------------------------------------------------------------
// - index : index of the transition (starts at 1).
// - pTable: pointer to the first transition.
// - numElements : number of elements in the pTable.
// - pState: pointer to the state of the transition.
// - pInterface: pointer to the interface of the transition.
// - pMsg: pointer to the message of the transition.
// - pTransFunc: pointer to the transition-function of the transition.
// - pNextState: pointer to the next state of the transition.
// -------------------------------------------------------------------------------------------------

SttElement_t* SetTransition
(
		int index,
		SttElement_t* pTable,
		int   numElements,
		void* pState,
		void* pInterface,
		void* pMsg,
		void *pTransFunc,
		void* pNextState
)
{
	// If this is the first transition.
	if( index == 1 ) return
			CreateTransition(Stt_State, pTable, numElements, pState, pInterface, pMsg, pTransFunc, pNextState );

	SttElement_t* pStateElement = pTable;
	SttElement_t* pInterfaceElement;
	SttElement_t* pMsgElement;

	// Loop through the states in the table.
	while( pStateElement != NULL )
	{
		if( pStateElement->pReference == pState )
		{
			// State found, find interface.
			pInterfaceElement = pStateElement+1;

			// Loop through the interfaces in the found state.
			while( pInterfaceElement != NULL )
			{
				if( pInterfaceElement->pReference == pInterface )
				{
					// Interface found.
					pMsgElement = pInterfaceElement+1;

					// Loop through the messages of the found interface to
					// check whether message not already handled.
					while( pMsgElement != NULL )
					{
						if( pMsgElement->pReference == pMsg )
						{
							// Message found --> double message --> ERROR
							// Transition not added.
							// TODO: Handle feedback for this error.
							return pTable; // Message found.
						}

						if( pMsgElement->pNextElement == NULL )
						{
							// This was the last message in this state/interface and therefore message
							// not found, which is how it should be at this point.
							pMsgElement->pNextElement = CreateTransition(Stt_Message, pTable, numElements, pState, pInterface, pMsg, pTransFunc, pNextState );
							return pTable;
						}

						// Move to consecutive message.
						pMsgElement = pMsgElement->pNextElement;
					}

					break; // Interface was found.
				}

				if( pInterfaceElement->pNextElement == NULL )
				{
					// This was the last interface in this state and therefore interface not found.
					pInterfaceElement->pNextElement = CreateTransition(Stt_Interface, pTable, numElements, pState, pInterface, pMsg, pTransFunc, pNextState );
					return pTable;
				}

				// Move to consecutive interface.
				pInterfaceElement = pInterfaceElement->pNextElement;
			}

			break; // State was found.
		}

		if( pStateElement->pNextElement == NULL )
		{
			// This was the last state and therefore state not found.
			pStateElement->pNextElement = CreateTransition(Stt_State, pTable, numElements, pState, pInterface, pMsg, pTransFunc, pNextState );
			return pTable;
		}

		// Move to consecutive state.
		pStateElement = pStateElement->pNextElement;
	}
	return pTable;
}

// -------------------------------------------------------------------------------------------------
// SetNextStates
// -------------------------------------------------------------------------------------------------
// When all the transitions are added, all 'next-states' are temporarily assumed to be
// dead-states. This must be corrected for those next-states that are not dead-states.
// -------------------------------------------------------------------------------------------------
// - pTable: pointer to the (first element) of the STT.
// - numelements : number of elements in the pTable.
// -------------------------------------------------------------------------------------------------
void SetNextStates(SttElement_t* pTable, int numElements )
{
	SttElement_t* pTransFuncElement = pTable;
	SttElement_t* pStateElement = pTable;

	SttElement_t* pState = pTable;
	SttElement_t* pInterface;
	SttElement_t* pMessage;

	// Loop through the table and find and process transition-function elements.

	while( pState != NULL )
	{
		// -- Loop through interfaces in this state --

		// Point to first interface in this state.
		pInterface = pState+1;
		while( pInterface != NULL )
		{
			// -- Loop through messages in this interface in this state --

			// Point to first message for this interface.
			pMessage = pInterface+1;
			while( pMessage != NULL )
			{
				// Point to transition function of this message.
				pTransFuncElement = pMessage+1;

				// -- Loop through all states --

				pStateElement = pTable;
				while( pStateElement != NULL )
				{
					if( pStateElement->pReference == pTransFuncElement->pNextElement)
					{
						// State found.
						pTransFuncElement->pNextElement = pStateElement;
						break;
					}
					// Move to consecutive state.
					pStateElement = pStateElement->pNextElement;
				}

				if( pStateElement == NULL )
				{
					// State not found.
					SttElement_t* pNextState = GetEmptyElement(pTable, 1, numElements);
					pNextState->pReference = pTransFuncElement->pNextElement;
					pNextState->pNextElement = NULL;
					pTransFuncElement->pNextElement = pNextState;
				}

				// Move to consecutive message.
				pMessage = pMessage->pNextElement;
			}
			// Move to consecutive interface.
			pInterface = pInterface->pNextElement;
		}
		// Move to consecutive state.
		pState = pState->pNextElement;
	}
}

