#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "fsm.h"
#include "stack.h"
#include "calculator.h"

#define PREVCHAR(x) (*(x - 1))
#define NEXTCHAR(x) (*(x + 1))
#define ISOPERATOR(x) ((x) == '+' || (x) == '-' || (x) == '*' || (x) == '/')
#define ISNUMBER(x) ((x) >= '0' && (x) <= '9')
#define ISOPEN_PARENTHESIS(x) ((x) == '(' || (x) == '[' || (x) == '{')
#define ISCLOSED_PARENTHESIS(x) ((x) == ')' || (x) == ']' || (x) == '}')
#define ISLEGAL_CHAR(x) (ISNUMBER(x) || ISOPERATOR(x) || ISOPEN_PARENTHESIS(x)\
													|| ISCLOSED_PARENTHESIS(x))
#define ISLOW_PRECEDENCE(x) ((x) == '+' || (x) == '-')
#define ISMEDIUM_PRECEDENCE(x) ((x) == '*' || (x) == '/')
#define STACK_PEEK(x) (*(char *)StackPeek(x))
#define IS_MATCHING_PARENTHESIS(x) \
		(	((x) == ')' && STACK_PEEK(p_stack) == '(') || \
		 	((x) == ']' && STACK_PEEK(p_stack) == '[') || \
		 	((x) == '}' && STACK_PEEK(p_stack) == '{')	)
#define DOUBLE(x) (2 * (x))
#define MAXPOSTFIXCHARS 200

typedef enum syntax_states
{
	___WAITING__,
	GOT___NUMBER,
	GOT_OPERATOR,
	GOT_CLOSED_P,
	____ERROR___,
	NUM_STATES
}syntax_states_t;

typedef enum syntax_symbols
{
	OPEN_P,
	NUMBER,
	OPERATOR,
	CLOSED_P,
	ILLEGAL_CHAR,
	NUM_SYMBOLS
}syntax_symbols_t;


int SymbolDecipher(char character)
{
	static char open_parenthesis[]   = "([{<";
	static char digit[] = "0123456789.";
	static char operators[] = "+-*/";
	static char closed_parenthesis[] = ")]}>";
	static char *symbols[] = {open_parenthesis, digit, operators,
															closed_parenthesis};
	unsigned int deciphered = 3;

	while (!strchr(symbols[deciphered], character) && deciphered > 0)
	{
		--deciphered;
	}

	return deciphered;
}

static int IsParenthesisCorrect(char *expression)
{
	unsigned int stack_is_empty = 0;
	unsigned int balance = 0;
	stack_t *p_stack = StackCreate(sizeof(char), strlen(expression));
	if (!p_stack)
	{
		return -1;
	}

	while (*expression)
	{
		if (ISOPEN_PARENTHESIS(*expression))
		{
			++balance;
			StackPush(p_stack, expression);
		}
		else if (ISCLOSED_PARENTHESIS(*expression))
		{
			--balance;
		}

		if (StackSize(p_stack))
		{
			if (IS_MATCHING_PARENTHESIS(*expression))
			{
				StackPop(p_stack);
			}	
		}

		++expression;
	}
	
	stack_is_empty = !(StackSize(p_stack));
	StackDestroy(p_stack);

	return (stack_is_empty && (0 == balance));
}

int IsSyntaxCorrect(char *expression)
{
	unsigned int result = 0;
	fsm_t *syntax_check = NULL;
	size_t num_states = NUM_STATES;
	size_t num_symbols = NUM_SYMBOLS;
	int current_state = ___WAITING__;
	const unsigned int accept_states[NUM_STATES] = {GOT___NUMBER, GOT_CLOSED_P};
	size_t num_accepted_states = 
							sizeof(accept_states) / sizeof(accept_states[0]);
	const transition_t syntax_trans_table[NUM_SYMBOLS][NUM_STATES] = 
	{
		{{___WAITING__, NULL}, {____ERROR___, NULL}, {___WAITING__, NULL}, {____ERROR___, NULL}, {____ERROR___, NULL}},
		{{GOT___NUMBER, NULL}, {GOT___NUMBER, NULL}, {GOT___NUMBER, NULL}, {____ERROR___, NULL}, {____ERROR___, NULL}},
		{{____ERROR___, NULL}, {GOT_OPERATOR, NULL}, {____ERROR___, NULL}, {GOT_OPERATOR, NULL}, {____ERROR___, NULL}},
		{{____ERROR___, NULL}, {GOT_CLOSED_P, NULL}, {GOT_CLOSED_P, NULL}, {GOT_CLOSED_P, NULL}, {____ERROR___, NULL}},
		{{____ERROR___, NULL}, {____ERROR___, NULL}, {____ERROR___, NULL}, {____ERROR___, NULL}, {____ERROR___, NULL}}
	};

	assert(expression);

	if (!IsParenthesisCorrect(expression))
	{
		return -1;
	}

	syntax_check = 
		FsmCreateWithUserParams((const transition_t *)syntax_trans_table,
		accept_states, num_accepted_states,num_states, num_symbols,
		current_state, NULL);

	while (*expression)
	{
		FsmNext(syntax_check, SymbolDecipher(*expression));
		++expression;
	}

	result = FsmIsAccepted(syntax_check);

	FsmDestroy(syntax_check);

	return result;
}

char *PostfixConverter(char *expression)
{
	size_t length = strlen(expression);
	stack_t *operator_stack = StackCreate(sizeof(char), length);
	char *ahead = NULL;
	char delimiter = ',';
	char *postfix = NULL;
	char *copy = calloc(DOUBLE(length), sizeof(char));
	if (!copy)
	{
		return NULL;
	}

	postfix = copy;

	while (*expression)
	{
		if (ISNUMBER(*expression))
		{
			strtod(expression, &ahead);
			memcpy(copy, expression, ahead-expression);
			copy += ahead - expression;
			*copy = delimiter;
			++copy;
			expression = ahead;
		}
		else if (ISOPERATOR(*expression))
		{
			if (StackSize(operator_stack) && 
				ISOPERATOR(STACK_PEEK(operator_stack)) &&
				ISLOW_PRECEDENCE(*expression))
			{
				*copy = STACK_PEEK(operator_stack);
				++copy;
				StackPop(operator_stack);
			}
			
			StackPush(operator_stack, expression);
			++expression;
		}
		else if (ISOPEN_PARENTHESIS(*expression))
		{
			StackPush(operator_stack, expression);
			++expression;
		}
		else if (ISCLOSED_PARENTHESIS(*expression))
		{
			while (StackSize(operator_stack) && 
					!ISOPEN_PARENTHESIS(STACK_PEEK(operator_stack)))
			{
				*copy = STACK_PEEK(operator_stack);
				++copy;
				*copy = delimiter;
				++copy;
				StackPop(operator_stack);
			}
			StackPop(operator_stack);
			++expression;
		}
	}

	while (StackSize(operator_stack))
	{
		*copy = STACK_PEEK(operator_stack);
		StackPop(operator_stack);
		++copy;
	}

	StackDestroy(operator_stack);

	return postfix;
}

double PostfixCalc(char *expression)
{
	double number1 = 0, number2 = 0;
	char *endptr = NULL;
	stack_t *number_stack = StackCreate(sizeof(double), strlen(expression));
	if (!number_stack)
	{
		return -1;
	}

	while (*expression)
	{
		if (ISOPERATOR((*expression)))
		{
			number2 = *(double *)StackPeek(number_stack);
			StackPop(number_stack);
			number1 = *(double *)StackPeek(number_stack);
			StackPop(number_stack);

			switch(*expression)
			{
				case '+':
				number1 += number2;
				break;

				case '-':
				number1 -= number2;
				break;

				case '*':
				number1 *= number2;
				break;

				case '/':
				number1 /= number2;
				break;
			}
			++expression;
			++expression;
			StackPush(number_stack, &number1);
		}
		else
		{
			number1 = strtod(expression, &endptr);
			expression = endptr;
			++expression;
			StackPush(number_stack, &number1);
		}
	}

	number1 = *(double *)StackPeek(number_stack);
	StackDestroy(number_stack);
	
	return number1;
}

double Calculate(char *expression)
{
	double result = 0;
	char arr[MAXPOSTFIXCHARS] = {0};
	char *to_be_freed = NULL;

	assert(expression);

	if (IsSyntaxCorrect(expression) != 1)
	{
		return -1;
	}
	
	to_be_freed = PostfixConverter(expression);
	memcpy(arr, to_be_freed, strlen(expression)*2);

	free(to_be_freed);

	result = PostfixCalc(arr);

	return result;
}

