#include <assert.h>
#include <stdlib.h>
#include "fsm.h"

#define TRANSITION_TABLE_OFFSET_POSITION ((transition_t *)fsm->transition_table\
					+ (symbol * fsm->number_of_states) + fsm->current_state)

struct fsm_t
{
	const transition_t *transition_table;
	const unsigned int *accept_states;
	size_t number_of_accept_states;
	size_t number_of_states;
	size_t number_of_symbols;
	unsigned int current_state;
	void *params;
};

fsm_t *FsmCreateWithUserParams(const transition_t *trans_table, 
	const unsigned int accept_states[],	size_t number_of_accept_states,
	size_t num_states, size_t num_symbols, unsigned int current_state, 
																void *params)
{
	fsm_t *fsm = NULL;

	assert(trans_table);
	assert(accept_states);
	assert(number_of_accept_states > 0);
	assert(num_states > 0);
	assert(num_symbols > 0);

	fsm = malloc(sizeof(fsm_t));
	if (!fsm)
	{
		return NULL;
	}

	fsm->transition_table = trans_table;
	fsm->accept_states = accept_states;
	fsm->number_of_accept_states = number_of_accept_states;
	fsm->number_of_states = num_states;
	fsm->number_of_symbols = num_symbols;
	fsm->current_state = current_state;
	fsm->params = params;

	return fsm;
}

void FsmDestroy(fsm_t *fsm)
{
	assert(fsm);
	
	free(fsm);
}

int FsmIsAccepted(fsm_t *fsm)
{
	unsigned int result = 0;
	unsigned int i = 0;

	assert(fsm);

	for(i = 0; i < fsm->number_of_accept_states; ++i)
	{
		result += (fsm->current_state == fsm->accept_states[i]);
	}

	return result;
}

void FsmNext(fsm_t *fsm, int symbol)
{
	assert(fsm);

	if (fsm->params)
	{
		TRANSITION_TABLE_OFFSET_POSITION->action_func(fsm->params);
	}
	
	fsm->current_state = TRANSITION_TABLE_OFFSET_POSITION->next_state;
}



