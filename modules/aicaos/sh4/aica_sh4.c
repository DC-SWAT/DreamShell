#include <kos.h>

#include "../aica_registers.h"
#include "../aica_syscalls.h"
#include "../aica_common.h"
#include "aica_sh4.h"

/* /!\ Invalid pointer - Do NOT deference it! */
static struct io_channel *io_addr_arm;
static unsigned int __io_init = 0xa09ffffc;

static kthread_t * thd;
static kthread_t * thd_create_idle(void);

/* Params:
 * 		out = function name (char *)
 * 		in = function ID (unsigned int *)
 */
static AICA_SHARED(__get_sh4_func_id)
{
	return aica_find_id((unsigned int *)out, (char *)in);
}

int aica_init(char *fn)
{
	thd = thd_create_idle();
	aica_clear_handler_table();

	g2_fifo_wait();
	g2_write_32(__io_init, 0);

	/* TODO: It would be faster to use mmap here,
	 * if the driver lies on the romdisk. */
	file_t file = fs_open(fn, O_RDONLY);

	if (file < 0)
		return -1;

	fs_seek(file, 0, SEEK_END);
	int size = fs_tell(file);
	fs_seek(file, 0, SEEK_SET);

	char* buffer = malloc(size);
	if (fs_read(file, buffer, size) < size) {
		fs_close(file);
		free(buffer);
		return -2;
	}

	/* We load the driver into the shared RAM. */
	spu_disable();
	aica_upload(0x0, buffer, size);
	spu_enable();

	fs_close(file);
	free(buffer);

	/* We wait until the ARM-7 writes at the address
	 * __io_init the message buffer's address. */
	do {
		g2_fifo_wait();
		io_addr_arm = (struct io_channel *) g2_read_32(__io_init);
	} while (!io_addr_arm);

	/* That function will be used by the remote processor to get IDs
	 * from the names of the functions to call. */
	AICA_SHARE(__get_sh4_func_id, FUNCNAME_MAX_LENGTH, 4);

	/* If the AICA_SHARED_LIST is used, we share all
	 * the functions it contains. */
	if (__aica_shared_list) {
		struct __aica_shared_function *ptr;
		for (ptr = __aica_shared_list; ptr->func; ptr++)
			__aica_share(ptr->func, ptr->name, ptr->sz_in, ptr->sz_out);
	}

	aica_init_syscalls();
	//	spu_dma_init();
	aica_interrupt_init();

	/* Notify the ARM that we are ready to receive calls. */
	g2_fifo_wait();
	g2_write_32(__io_init, 0);

	printf("ARM successfully initialized.\n");
	return 0;
}

void aica_exit(void)
{
	asic_evt_disable(ASIC_EVT_SPU_IRQ, ASIC_IRQ9);
	spu_disable();
	aica_clear_handler_table();
}

int __aica_call(unsigned int id, void *in, void *out, unsigned short prio)
{
	struct function_params fparams;
	struct call_params cparams;
	int return_value;

	if (id >= NB_MAX_FUNCTIONS)
		return -EINVAL;

	while (1) {
		/* We retrieve the parameters of the function we want to execute */
		aica_download(&fparams,
					&io_addr_arm[SH_TO_ARM].fparams[id], sizeof(fparams));

		/* We don't want two calls to the same function
		 * to occur at the same time. */
		if (fparams.call_status == FUNCTION_CALL_AVAIL)
			break;
		thd_pass();
	}

	fparams.call_status = FUNCTION_CALL_PENDING;
	aica_upload(&io_addr_arm[SH_TO_ARM].fparams[id], &fparams, sizeof(fparams));

	/* We will start transfering the input buffer. */
	if (fparams.in.size > 0)
		aica_upload(fparams.in.ptr, in, fparams.in.size);

	/* Wait until a new call can be made. */
	while (1) {
		aica_download(&cparams,
					&io_addr_arm[SH_TO_ARM].cparams, sizeof(cparams));
		if (!cparams.sync)
			break;
		thd_pass();
	}

	/* Fill the I/O structure with the call parameters, and submit the new call.
	 * That function will return immediately, even if the remote function
	 * has yet to be executed. */
	cparams.id = id;
	cparams.prio = prio;
	cparams.sync = 1;
	aica_upload(&io_addr_arm[SH_TO_ARM].cparams, &cparams, sizeof(cparams));
	aica_interrupt();

	/* Wait until the call completes to transfer the data. */
	while (1) {
		aica_download(&fparams,
					&io_addr_arm[SH_TO_ARM].fparams[id], sizeof(fparams));
		if (fparams.call_status == FUNCTION_CALL_DONE)
			break;
		thd_pass();
	}

	if (fparams.out.size > 0)
		aica_download(out, fparams.out.ptr, fparams.out.size);
	return_value = fparams.return_value;

	/* Set the 'errno' variable to the value returned by the ARM */
	errno = fparams.err_no;

	/* Mark the function as available */
	fparams.call_status = FUNCTION_CALL_AVAIL;
	aica_upload(&io_addr_arm[SH_TO_ARM].fparams[id], &fparams, sizeof(fparams));
	return return_value;
}

static void * aica_arm_fiq_hdl_thd(void *param)
{
	struct call_params cparams;
	struct function_params fparams;

	/* Create a new idle thread to handle next request */
	thd = thd_create_idle();

	/* Retrieve the call parameters */
	aica_download(&cparams, &io_addr_arm[ARM_TO_SH].cparams, sizeof(cparams));

	/* The call data has been read, clear the sync flag and acknowledge. */
	cparams.sync = 0;
	aica_upload(&io_addr_arm[ARM_TO_SH].cparams, &cparams, sizeof(cparams));

	thd_set_prio(thd_get_current(), cparams.prio);

	/* Download information about the function to call */
	aica_download(&fparams,
				&io_addr_arm[ARM_TO_SH].fparams[cparams.id], sizeof(fparams));

	/* Download the input data if any. */
	if (fparams.in.size > 0)
		aica_download(fparams.in.ptr, cparams.in, fparams.in.size);

	/* Get a handle from the ID. */
	aica_funcp_t func = aica_get_func_from_id(cparams.id);
	if (!func) {
		fprintf(stderr, "No function found for ID %i.\n", cparams.id);
		free(param);
		return NULL;
	}

	/* Call the function. */
	fparams.return_value = (*func)(fparams.out.ptr, fparams.in.ptr);

	/* Transfer the 'errno' variable to the ARM */
	fparams.err_no = errno;

	/* Upload the output data. */
	if (fparams.out.size > 0)
		aica_upload(cparams.out, fparams.out.ptr, fparams.out.size);

	/* Inform the ARM that the call is complete. */
	fparams.call_status = FUNCTION_CALL_DONE;
	aica_upload(&io_addr_arm[ARM_TO_SH].fparams[cparams.id],
				&fparams, sizeof(fparams));

	return NULL;
}

static kthread_t * thd_create_idle(void)
{
	int irq_status = irq_disable();

	kthread_t * thread = thd_create(THD_DETACHED, aica_arm_fiq_hdl_thd, NULL);
	thd_remove_from_runnable(thread);
	thd_set_prio(thread, 0);

	irq_restore(irq_status);
	return thread;
}

static void acknowledge(void)
{
	g2_write_32(AICA_FROM_SH4(REG_SH4_INT_RESET), MAGIC_CODE);
}

static void aica_arm_fiq_hdl(uint32_t code)
{
	thd_add_to_runnable(thd, 0);
	acknowledge();
}

void aica_interrupt_init(void)
{
	/* Cancel any pending interrupt. */
	acknowledge();

	asic_evt_set_handler(ASIC_EVT_SPU_IRQ, aica_arm_fiq_hdl);
	asic_evt_enable(ASIC_EVT_SPU_IRQ, ASIC_IRQ9);
}

void aica_interrupt(void)
{
	g2_write_32(AICA_FROM_SH4(REG_ARM_INT_SEND), MAGIC_CODE);
}

void aica_update_fparams_table(unsigned int id, struct function_params *fparams)
{
	aica_upload(&io_addr_arm[ARM_TO_SH].fparams[id], fparams, sizeof(*fparams));
}

void aica_upload(void *dest, const void *from, size_t size)
{
	spu_memload((unsigned int)dest, (void *)from, size);
}

void aica_download(void *dest, const void *from, size_t size)
{
	spu_memread(dest, (unsigned int)from, size);
}
