// -------------------------------------------------------------------------------------------------
// Timer.c
// -------------------------------------------------------------------------------------------------

#include "Timer.h"

// -------------------------------------------------------------------------------------------------
// -- Timer_init --
// -------------------------------------------------------------------------------------------------
// pInterface: pointer to the Timer interface.
// -------------------------------------------------------------------------------------------------
void Timer_init( Timer_t* pInterface)
{
	SET_MESSAGE_ID(TimeoutInd);
	pInterface->pName = "Timer";
}
