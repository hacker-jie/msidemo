// SPDX-License-Identifier: GPL-2.0
/*
 * Testing MSI with e1000e
 *
 * Jie Deng <mr.dengjie@gmail.com>
 */

#include <linux/module.h>	// for init_module() 
#include <linux/proc_fs.h>	// for create_proc_info_entry() 
#include <linux/pci.h>		// for pci_get_device()
#include <linux/interrupt.h>	// for request_irq()
#include <linux/seq_file.h>	// for sequence files

#define VENDOR_ID	0x8086	// Intel Corporation
#define DEVICE_ID	0x156f	// E1000_DEV_ID_PCH_SPT_I219_LM
#define irqID		18	// temporary -- an unused IRQ
#define intID		18	// temporary -- IOAPIC mapped


enum {
	E1000_ICR	= 0x00C0,	// Interrupt Cause Read
	E1000_ICS	= 0x00C8,	// Interrupt Cause Set
	E1000_IMS	= 0x00D0,	// Interrupt Mask Set
	E1000_IMC	= 0x00D8,	// Interrupt Mask Clear
};

char modname[] = "msidemo";
struct pci_dev	*devp;
unsigned int	mmio_base;
unsigned int	mmio_size;
void		*io;
wait_queue_head_t  my_wq;
int		irqcount = 0;
u32		msi_cap[4];
int 		irq;

static void print_msi_info(struct pci_dev  *devp)
{
        static int i = 0;
        u32 msi_data[4];

        if (devp == NULL)
                return;

        pci_read_config_dword(devp, 0xD0, &msi_data[0]);
        pci_read_config_dword(devp, 0xD4, &msi_data[1]);
        pci_read_config_dword(devp, 0xD8, &msi_data[2]);
        pci_read_config_dword(devp, 0xDC, &msi_data[3]);

        printk("%d. msi ctrl=%x\n", ++i, msi_data[0]);
        printk("%d. msi addr=%x\n", i,   msi_data[1]);
        printk("%d. msi data=%x\n", i,   msi_data[3]);
}

static void seq_print_msi_info(struct pci_dev  *devp, struct seq_file *m)
{
        static int i = 0;
        u32 msi_data[4];

        if (devp == NULL)
                return;

        pci_read_config_dword(devp, 0xD0, &msi_data[0]);
        pci_read_config_dword(devp, 0xD4, &msi_data[1]);
        pci_read_config_dword(devp, 0xD8, &msi_data[2]);
        pci_read_config_dword(devp, 0xDC, &msi_data[3]);

        seq_printf(m, "%d. msi ctrl=%x\n", ++i, msi_data[0]);
        seq_printf(m, "%d. msi addr=%x\n", i,   msi_data[1]);
        seq_printf(m, "%d. msi data=%x\n", i,   msi_data[3]);
}

static int msidemo_show(struct seq_file *m, void *v)
{
	int oldcount;

	seq_printf(m, "proir irqcount = %d\n", irqcount);
	seq_printf(m, "triggering another MSI\n");

	oldcount = irqcount;
        iowrite32(1<<4, io + E1000_ICS);
        wait_event_interruptible(my_wq, irqcount != oldcount);

	seq_print_msi_info(devp, m);

	return 0;
}

static int msidemo_open(struct inode *inode, struct file *file)
{
	return single_open(file, msidemo_show, NULL);
}

static const struct file_operations msidemo_ops = {
	.open	 = msidemo_open,
	.read	 = seq_read,
	.llseek	 = seq_lseek,
	.release = single_release,
};

irqreturn_t my_isr(int irq, void *dev_id)
{
	int interrupt_cause = ioread32(io + E1000_ICR);
	if (interrupt_cause == 0)
		return IRQ_NONE;

	++irqcount;

	printk("MSIDEMO: irq=%02X  interrupt #%d  ", irq, irqcount); 
	printk("e1000e ICR=%08X \n", interrupt_cause);

	wake_up_interruptible(&my_wq);
	iowrite32(interrupt_cause, io + E1000_ICR);
	return IRQ_HANDLED;
}

static int __init msidemo_init(void)
{
	u16 pci_cmd;
	u32 msi_ctrl, msi_addr, msi_data;
	int ret; 

	printk("<1>\nInstalling \'%s\' module\n", modname);

	// detect the presence of the I219-LM network controller
	devp = pci_get_device(VENDOR_ID, DEVICE_ID, NULL);
	if (!devp)
		return -ENODEV;

	print_msi_info(devp);

        // save contents the I219's MSI Capability Register Set 
        pci_read_config_dword(devp, 0xD0, &msi_cap[0]);
        pci_read_config_dword(devp, 0xD4, &msi_cap[1]);
        pci_read_config_dword(devp, 0xD8, &msi_cap[2]);
        pci_read_config_dword(devp, 0xDC, &msi_cap[3]);

	//enable pci device
	ret = pcim_enable_device(devp);
	if (ret < 0) {
        	printk("enable pci device fail, ret=%d\n", ret);
                return ret;
        }
	
	print_msi_info(devp);

	// map controller's i/o-memory into kernel's address-space
	mmio_base = pci_resource_start(devp, 0);
	mmio_size = pci_resource_len(devp, 0);
	io = ioremap_nocache(mmio_base, mmio_size);
	if (!io)
		return -ENOSPC;
#if 0
	// insure the I219-LM has its Bus Master capability enabled
	pci_read_config_word(devp, 4, &pci_cmd);
	pci_cmd |= (1 << 2);			// enable Bus Master
	pci_write_config_word(devp, 4, pci_cmd);

	// configure the I219 to generate Message Signaled Interrupts
	msi_data = 0x4000 | intID;
	msi_addr = 0xFEE002f8;
	msi_ctrl = msi_cap[0] | 0x00010000;
	pci_write_config_dword(devp, 0xDC, msi_data);
	pci_write_config_dword(devp, 0xD4, msi_addr);
	pci_write_config_dword(devp, 0xD0, msi_ctrl);
	irq = irqID;

        print_msi_info(devp);
#endif

	// initialize our module's wait-queue 
	init_waitqueue_head(&my_wq);

#if 1
	// Get irq
	ret = pci_alloc_irq_vectors(devp, 1, 1, PCI_IRQ_MSI);
	if (ret < 0) {
		printk("alloc IRQ_MSI fail, ret=%d\n", ret);
		return ret;
	}
	irq = pci_irq_vector(devp, 0);
	printk("irq = %d\n", irq);

	print_msi_info(devp);
#endif

	// install interrupt-handler and enable I219 interrupts
	if (request_irq(irq, my_isr, IRQF_SHARED, modname, &modname ) < 0) {
		iounmap(io);
		return -EBUSY;
	}
	iowrite32(0xFFFFFFFF, io + E1000_IMS);

	// create this module's pseudo-file
	proc_create(modname, 0, NULL, &msidemo_ops);

	printk("End of msidemo\n");

	return	0;  //SUCCESS
}


static void __exit msidemo_exit(void)
{
	// delete this module's pseudo-file
	remove_proc_entry(modname, NULL);

	// disable I219 interrupts and remove interrupt-handler
	iowrite32(0xFFFFFFFF, io + E1000_IMC);
	free_irq(irq, modname);
	pci_free_irq_vectors(devp);	

	// restore original values to the MSI Capability Register Set
	pci_write_config_dword(devp, 0xD0, msi_cap[0]);
	pci_write_config_dword(devp, 0xD4, msi_cap[1]);
	pci_write_config_dword(devp, 0xD8, msi_cap[2]);
	pci_write_config_dword(devp, 0xDC, msi_cap[3]);

	// unmap the I219's i/o-memory
	iounmap(io);

	printk("<2>Removing \'%s\' module\n", modname);
}

module_init(msidemo_init);
module_exit(msidemo_exit);
MODULE_LICENSE("GPL");
