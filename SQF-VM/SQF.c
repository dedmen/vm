#include "SQF.h"
#include "SQF_types.h"

#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


PVM sqfvm(unsigned int stack_size, unsigned int work_size, unsigned int cmds_size)
{
	PVM vm = malloc(sizeof(VM));
	vm->stack = create_stack(stack_size);
	vm->work = create_stack(work_size);

	vm->cmds = malloc(sizeof(PCMD) * cmds_size);
	memset(vm->cmds, 0, sizeof(PINST) * cmds_size);
	vm->cmds_size = cmds_size;
	vm->cmds_top = 0;


	register_command(vm, create_command("SCALAR", 't', 0, 0));
	register_command(vm, create_command("BOOL", 't', 0, 0));

	register_command(vm, CODE_TYPE());
	register_command(vm, STRING_TYPE());
	return vm;
}
void destroy_sqfvm(PVM vm)
{
	int i;

	for (i = 0; i < vm->cmds_top; i++)
	{
		destroy_command(vm->cmds[i]);
	}
	free(vm->cmds);

	destroy_stack(vm->stack);
	destroy_stack(vm->work);
	free(vm);
}

PSTACK create_stack(unsigned int size)
{
	PSTACK stack = malloc(sizeof(STACK));
	stack->data = malloc(sizeof(PINST) * size);
	memset(stack->data, 0, sizeof(PINST) * size);
	stack->size = size;
	stack->top = 0;
}
void destroy_stack(PSTACK stack)
{
	int i;
	for (i = 0; i < stack->top; i++)
	{
		inst_destroy(stack->data[i]);
	}
	free(stack);
}
void resize_stack(PSTACK stack, unsigned int newsize)
{
	if (stack->size == newsize)
		return;
	int i;
	PINST** ptr;
	for (i = newsize; i < stack->top; i++)
	{
		inst_destroy(stack->data[i]);
	}
	ptr = realloc(stack->data, sizeof(PINST) * newsize);
	if (ptr == 0)
	{
		error("RESIZE OF STACK FAILED", stack);
	}
	else
	{
		stack->data = ptr;
		stack->size = newsize;
	}
}

PCMD create_command(const char* name, char type, CMD_CB fnc, char precedence)
{
	PCMD command = malloc(sizeof(CMD));
	int len = strlen(name);
	command->name = malloc(sizeof(char) * (len + 1));
	command->name_len = len;
	strcpy(command->name, name);
	command->precedence_level = precedence;
	command->type = type;
	command->type_code = type == 't' ? 0 : type == 'b' ? 1 : type == 'u' ? 2 : 3;
	command->callback = fnc;
	return command;
}
void destroy_command(PCMD command)
{
	free(command->name);
	free(command);
}

PVALUE find_var(PVM vm, const char* name)
{
	int i, j;
	PSCOPE scope;
	for (i = vm->stack->top; i >= 0; i--)
	{
		if (vm->stack->data[i]->type == INST_SCOPE)
		{
			scope = get_scope(vm, vm->stack->data[i]);
			for (j = 0; j < scope->varstack_top; j++)
			{
				if (strcmpi(scope->varstack_name[j], name) == 0)
				{
					return scope->varstack_value[j];
				}
			}
		}
	}
	return 0;
}
void set_var(PVM vm, const char* name, VALUE val)
{
	int i, j;
	PSCOPE first = 0;
	PSCOPE scope;
	for (i = vm->stack->top; i >= 0; i--)
	{
		if (vm->stack->data[i]->type == INST_SCOPE)
		{
			scope = get_scope(vm, vm->stack->data[i]);
			if (first == 0)
			{
				first = scope;
			}
			for (j = 0; j < scope->varstack_top; j++)
			{
				if (strcmpi(scope->varstack_name[j], name) == 0)
				{
					inst_destroy_value(scope->varstack_value[j]);
					scope->varstack_value[j] = malloc(sizeof(VALUE));
					scope->varstack_value[j]->type = val.type;
					scope->varstack_value[j]->val = val.val;
				}
			}
		}
	}
	store_in_scope(vm, first, name, val);
}
PSCOPE top_scope(PVM vm)
{
	int i;
	for (i = vm->stack->top; i >= 0; i--)
	{
		if (vm->stack->data[i]->type == INST_SCOPE)
		{
			return get_scope(vm, vm->stack->data[i]);
		}
	}
	return 0;
}
void store_in_scope(PVM vm, PSCOPE scope, const char* name, VALUE val)
{
	int len;
	if (scope->varstack_top >= scope->varstack_size)
	{
		error("VARSTACK OVERFLOW", vm->stack);
	}
	else
	{
		len = strlen(name);
		scope->varstack_name[scope->varstack_top] = malloc(sizeof(char) * (len + 1));
		strcpy(scope->varstack_name[scope->varstack_top], name);

		scope->varstack_value[scope->varstack_top] = malloc(sizeof(VALUE));
		scope->varstack_value[scope->varstack_top]->type = val.type;
		scope->varstack_value[scope->varstack_top]->val = val.val;

		scope->varstack_top++;
	}
}

PCMD find_command(PVM vm, const char* name, char type)
{
	int i;
	PCMD cmd;
	if (type != '*')
	{
		for (i = 0; i < vm->cmds_top; i++)
		{
			cmd = vm->cmds[i];
			if (cmd->type == type && strcmpi(cmd->name, name) == 0)
			{
				return cmd;
			}
		}
	}
	else
	{
		for (i = 0; i < vm->cmds_top; i++)
		{
			cmd = vm->cmds[i];
			if (strcmpi(cmd->name, name) == 0)
			{
				return cmd;
			}
		}
	}
	return 0;
}
PCMD find_type(PVM vm, const char* name)
{
	int i;
	PCMD cmd;
	for (i = 0; i < vm->cmds_top; i++)
	{
		cmd = vm->cmds[i];
		if (strcmpi(cmd->name, name) == 0)
		{
			return cmd;
		}
		else if (cmd->type != 't')
		{
			break;
		}
	}
	return 0;
}

void error(const char* errMsg, PSTACK stack)
{
	printf("ERROR: %s\n", errMsg);
	__asm int 3;
}

void execute(PVM vm)
{
	PINST inst;
	PINST inst2;
	PVALUE val;
	PVALUE val2;
	PSCOPE scope;
	while (vm->stack->top > 0)
	{
		inst = pop_stack(vm->stack);
		switch (inst->type)
		{
		case INST_NOP:
			inst_destroy(inst);
			break;
		case INST_COMMAND:
			get_command(vm->stack, inst)->callback(vm);
			inst_destroy(inst);
			break;
		case INST_SCOPE:
			inst_destroy(inst);
			break;
		case INST_LOAD_VAR:
			val = find_var(vm, get_var_name(vm->stack, inst));
			if (val == 0)
			{
				push_stack(vm->work, inst_nop());
			}
			else
			{
				push_stack(vm->work, inst_value(value(val->type, val->val)));
			}
			inst_destroy(inst);
			break;
		case INST_STORE_VAR:
			val = find_var(vm, get_var_name(vm->stack, inst));
			inst2 = pop_stack(vm->work);
			if (val == 0)
			{
				scope = top_scope(vm);
				val = get_value(vm->stack, inst2);
				store_in_scope(vm, scope, get_var_name(vm->stack, inst), value(val->type, val->val));
			}
			else
			{
				val2 = get_value(vm, inst2);
				set_var(vm, get_var_name(vm->stack, inst), value(val2->type, val2->val));
			}
			inst_destroy(inst2);
			inst_destroy(inst);
			break;
		case INST_STORE_VAR_LOCAL:
			scope = top_scope(vm);
			inst2 = pop_stack(vm->work);
			val = get_value(vm->stack, inst2);
			store_in_scope(vm, scope, get_var_name(vm->stack, inst), value(val->type, val->val));
			inst_destroy(inst2);
			inst_destroy(inst);
			break;
		case INST_VALUE:
			push_stack(vm->work, inst);
			break;
		}
	}
}
