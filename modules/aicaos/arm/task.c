
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <reent.h>
#include <sys/queue.h>

#include "../aica_common.h"
#include "task.h"
#include "interrupt.h"

struct task *current_task;
static unsigned int id = 0;

struct TaskHandler {
	struct task * task;
	SLIST_ENTRY(TaskHandler) next;
};

static SLIST_HEAD(Head, TaskHandler) tasks [PRIORITY_MAX+1];

void task_select(struct task *task)
{
	/* Inside task_asm.S */
	void __task_select(struct context *context);

	int_disable();
	current_task = task;
	_impure_ptr = &task->reent;
	__task_select(&task->context);
}

/* Called from task_asm.S */
void __task_reschedule()
{
	uint32_t i;
	struct TaskHandler *hdl;

	int_disable();
	for (i = 0; i <= PRIORITY_MAX; i++) {
		SLIST_FOREACH(hdl, &tasks[i], next) {
			if (hdl->task != current_task)
				task_select(hdl->task);
		}
	}

	task_select(current_task);
}

void task_exit(void)
{
	struct TaskHandler *hdl;
	unsigned int i;

	int_disable();
	for (i = 0; i <= PRIORITY_MAX; i++) {
		SLIST_FOREACH(hdl, &tasks[i], next) {
			if (hdl->task == current_task) {

				/* Revert to the main stack */
				__asm__ volatile("ldr sp,=__stack");

				SLIST_REMOVE(&tasks[i], hdl, TaskHandler, next);
				free(hdl->task->stack);
				free(hdl->task);
				free(hdl);

				__task_reschedule();
			}
		}
	}
}

struct task * task_create(void *func, void *param[4])
{
	struct task *task = malloc(sizeof(struct task));
	if (!task)
		return NULL;

	/* Init the stack */
	task->stack_size = DEFAULT_STACK_SIZE;
	task->stack = malloc(DEFAULT_STACK_SIZE);
	if (!task->stack) {
		free(task);
		return NULL;
	}

	if (param)
		memcpy(task->context.r0_r7, param, 4 * sizeof(void *));
	task->context.r8_r14[5] = (uint32_t) task->stack + DEFAULT_STACK_SIZE;
	task->context.r8_r14[6] = (uint32_t) task_exit;
	task->context.pc = func;
	task->context.cpsr = 0x13; /* supervisor */

	/* Init newlib's reent structure */
	_REENT_INIT_PTR(&task->reent);

	task->id = id++;
	return task;
}

void task_add_to_runnable(struct task *task, unsigned char prio)
{
	uint32_t cxt = int_disable();
	struct TaskHandler *old = NULL,
					   *new = malloc(sizeof(struct TaskHandler)),
					   *hdl = SLIST_FIRST(&tasks[prio]);
	new->task = task;

	SLIST_FOREACH(hdl, &tasks[prio], next)
		old = hdl;

	if (old)
		SLIST_INSERT_AFTER(old, new, next);
	else
		SLIST_INSERT_HEAD(&tasks[prio], new, next);
	int_restore(cxt);
}

