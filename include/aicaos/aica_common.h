#ifndef _AICA_H
#define _AICA_H

#include <sys/types.h>
#include <errno.h>

#define SH_TO_ARM 0
#define ARM_TO_SH 1

#define NB_MAX_FUNCTIONS 0x40

#define FUNCNAME_MAX_LENGTH 0x20

#define PRIORITY_MAX 15

/* Default priority for a function call */
#define PRIORITY_DEFAULT 10

/* EAICA error for 'errno'.
 * It means that the function call did not success. */
#define EAICA 101

#define FUNCTION_CALL_AVAIL		1
#define FUNCTION_CALL_PENDING	2
#define FUNCTION_CALL_DONE		3

typedef int (*aica_funcp_t)(void *, void *);

struct function_flow_params
{
	/* Length in bytes of the buffer. */
	size_t size;

	/* Pointer to the buffer.
	 * /!\ Can be inaccessible from the ARM! */
	void *ptr;
};

struct function_params
{
	/* Input and output buffers of the function. */
	struct function_flow_params in;
	struct function_flow_params out;

	/* Indicates whether the previous call of that function
	 * is still pending or has been completed. */
	int call_status;

	/* Integer value returned by the function */
	int return_value;

	/* Contains the error code (if error there is) */
	int err_no;
};

/* Contains the parameters relative to one
 * function call. */
struct call_params
{
	/* ID of the function to call */
	unsigned int id;

	/* Priority of the call */
	unsigned short prio;

	/* Flag value: set when the io_channel is
	 * free again. */
	unsigned char sync;

	/* local addresses where to load/store results */
	void *in;
	void *out;
};

/* That structure defines one channel of communication.
 * Two of them will be instancied: one for the
 * sh4->arm channel, and one for the arm->sh4 channel. */
struct io_channel
{
	/* Params of the call. */
	struct call_params cparams;

	/* Array of function params.
	 * fparams[i] contains the params for the
	 * function with the ID=i. */
	struct function_params fparams[NB_MAX_FUNCTIONS];
};

/* Make a function available to the remote processor.
 * /!\ Never use that function directly!
 * Use the macro AICA_SHARE instead. */
void __aica_share(aica_funcp_t func,
			const char *funcname, size_t sz_in, size_t sz_out);

/* Call a function shared by the remote processor.
 * /!\ Never use that function directly!
 * Use the AICA_ADD_REMOTE macro instead; it will
 * define locally the remote function. */
int __aica_call(unsigned int id,
			void *in, void *out, unsigned short prio);

/* Unregister the handler at the given ID. */
int aica_clear_handler(unsigned int id);

/* Clear the whole table. */
void aica_clear_handler_table(void);

/* Update the function params table. */
void aica_update_fparams_table(unsigned int id,
			struct function_params *fparams);

/* Return the ID associated to a function name. */
int aica_find_id(unsigned int *id, char *funcname);

/* Return the function associated to an ID. */
aica_funcp_t aica_get_func_from_id(unsigned int id);

/* Return the name of the function associated to an ID. */
const char * aica_get_funcname_from_id(unsigned int id);

/* Send data to the remote processor. */
void aica_upload(void *dest, const void *from, size_t size);

/* Receive data from the remote processor. */
void aica_download(void *dest, const void *from, size_t size);

/* Initialize the interrupt system. */
void aica_interrupt_init(void);

/* Send a notification to the remote processor. */
void aica_interrupt(void);

/* Stop all communication with the AICA subsystem. */
void aica_exit(void);


struct __aica_shared_function {
	aica_funcp_t func;
	const char *name;
	size_t sz_in, sz_out;
};

#define AICA_SHARED_LIST struct __aica_shared_function __aica_shared_list[]
extern AICA_SHARED_LIST __attribute__((weak));

#define AICA_SHARED_LIST_ELEMENT(func, sz_in, sz_out) \
	{ func, #func, sz_in, sz_out, }

#define AICA_SHARED_LIST_END \
	{ NULL, NULL, 0, 0, }

#define AICA_SHARE(func, sz_in, sz_out) \
	__aica_share(func, #func, sz_in, sz_out)

#define AICA_SHARED(func) \
	int func(void *out, void *in)

#define AICA_ADD_REMOTE(func, prio) \
	static int _##func##_id = -1; \
	int func(void *out, void *in) \
	{ \
		if (_##func##_id < 0) { \
			int res = __aica_call(0, #func, &_##func##_id, 0); \
			if (res < 0) \
				return res; \
		} \
		return __aica_call(_##func##_id, in, out, prio); \
	}

#endif
