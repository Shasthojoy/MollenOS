/* MollenOS
*
* Copyright 2011 - 2014, Philip Meulengracht
*
* This program is free software : you can redistribute it and / or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation ? , either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.If not, see <http://www.gnu.org/licenses/>.
*
*
* MollenOS x86-32 AP Setup
*/

#include <Arch.h>
#include <Acpi.h>
#include <Apic.h>
#include <SysTimers.h>
#include <Gdt.h>
#include <Thread.h>
#include <Scheduler.h>
#include <Memory.h>
#include <Idt.h>
#include <Cpu.h>
#include <string.h>
#include <stdio.h>
#include <List.h>

/* Externs */
extern list_t *GlbAcpiNodes;
extern volatile uint8_t GlbBootstrapCpuId;
extern x86CpuObject_t GlbBootCpuInfo;

/* These two are located in boot.asm */
extern void CpuEnableSse(void);
extern void CpuEnableFpu(void);

/* Globals */
Spinlock_t GlbApLock;
volatile uint32_t GlbCpusBooted = 1;

/* Trampoline Code */
unsigned char TrampolineCode[] = {
	0xE9, 0xC4, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x10,
	0xEF, 0xBE, 0xAD, 0xDE, 0x00, 0x20, 0x00, 0x00, 0x9C, 0x1E, 0x06,
	0x57, 0x56, 0xFA, 0x31, 0xC0, 0x8E, 0xC0, 0xF7, 0xD0, 0x8E, 0xD8,
	0xBF, 0x00, 0x05, 0xBE, 0x10, 0x05, 0x26, 0x8A, 0x05, 0x50, 0x3E,
	0x8A, 0x04, 0x50, 0x26, 0xC6, 0x05, 0x00, 0x3E, 0xC6, 0x04, 0xFF,
	0x26, 0x80, 0x3D, 0xFF, 0x58, 0x3E, 0x88, 0x04, 0x58, 0x26, 0x88,
	0x05, 0xB8, 0x00, 0x00, 0x74, 0x03, 0xB8, 0x01, 0x00, 0x5E, 0x5F,
	0x07, 0x1F, 0x9D, 0xC3, 0x60, 0xE8, 0x2E, 0x00, 0xB0, 0xAD, 0xE6,
	0x64, 0xE8, 0x27, 0x00, 0xB0, 0xD0, 0xE6, 0x64, 0xE8, 0x27, 0x00,
	0xE4, 0x60, 0x66, 0x50, 0xE8, 0x19, 0x00, 0xB0, 0xD1, 0xE6, 0x64,
	0xE8, 0x12, 0x00, 0x66, 0x58, 0x0C, 0x02, 0xE6, 0x60, 0xE8, 0x09,
	0x00, 0xB0, 0xAE, 0xE6, 0x64, 0xE8, 0x02, 0x00, 0x61, 0xC3, 0xE4,
	0x64, 0xA8, 0x02, 0x75, 0xFA, 0xC3, 0xE4, 0x64, 0xA8, 0x01, 0x74,
	0xFA, 0xC3, 0x60, 0x0F, 0x01, 0x16, 0xC1, 0x50, 0x61, 0xC3, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00,
	0x00, 0x9A, 0xCF, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x92, 0xCF,
	0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0xFA, 0xCF, 0x00, 0xFF, 0xFF,
	0x00, 0x00, 0x00, 0xFA, 0xCF, 0x00, 0x27, 0x00, 0x99, 0x50, 0x00,
	0x00, 0xFA, 0x66, 0x31, 0xC0, 0x66, 0x31, 0xDB, 0x66, 0x31, 0xC9,
	0x8E, 0xD8, 0x8E, 0xC0, 0x66, 0x8B, 0x0E, 0x0F, 0x50, 0xF0, 0x66,
	0x0F, 0xC1, 0x0E, 0x03, 0x50, 0x66, 0x03, 0x0E, 0x0F, 0x50, 0x66,
	0x89, 0xCA, 0x89, 0xCC, 0x66, 0xC1, 0xE9, 0x10, 0x66, 0xC1, 0xE1,
	0x0C, 0x8E, 0xD1, 0x66, 0x52, 0x66, 0x31, 0xDB, 0x66, 0x31, 0xC9,
	0xE8, 0x13, 0xFF, 0x3D, 0x01, 0x00, 0x74, 0x03, 0xE8, 0x49, 0xFF,
	0xE8, 0x86, 0xFF, 0x0F, 0x20, 0xC0, 0x66, 0x0D, 0x01, 0x00, 0x00,
	0x00, 0x0F, 0x22, 0xC0, 0x66, 0x59, 0xEA, 0x1E, 0x51, 0x08, 0x00,
	0x66, 0xB8, 0x10, 0x00, 0x8E, 0xD8, 0x8E, 0xC0, 0x8E, 0xD0, 0x89,
	0xCC, 0xFC, 0xA1, 0x0B, 0x50, 0x00, 0x00, 0xFF, 0xD0
};

/* Entry for AP Cores */
void SmpApEntry(void)
{
	Cpu_t Cpu;

	/* Disable interrupts */
	InterruptDisable();

	/* Install GDT, IDT */
	GdtInstall();
	IdtInstall();

	/* Enable FPU */
	if (GlbBootCpuInfo.EdxFeatures & CPUID_FEAT_EDX_FPU)
		CpuEnableFpu();

	/* Enable SSE */
	if (GlbBootCpuInfo.EdxFeatures & CPUID_FEAT_EDX_SSE)
		CpuEnableSse();
	
	/* TSS */
	GdtInstallTss();

	/* Get Apic Id */
	Cpu = (ApicReadLocal(APIC_PROCESSOR_ID) >> 24) & 0xFF;

	/* Memory */
	MmVirtualInstallPaging(Cpu);

	/* Setup apic */
	printf("Cpu (%u) - 1\n", Cpu);
	ApicInitAp();
	printf("Cpu (%u) - 2\n", Cpu);

	/* Setup Threading */
	SchedulerInit(Cpu);
	printf("Cpu (%u) - 3\n", Cpu);
	ThreadingApInit();
	printf("Cpu (%u) - 4\n", Cpu);

	/* Increament Boot Count - Signal that we are ok */
	GlbCpusBooted++;

	/* Dont ever go out of this function */
	InterruptEnable();

	while (1)
		Idle();
}

/* Disable MSVC warning */
#pragma warning(disable:4054)

/* Setup Trampoline Code */
void SmpApSetup(void)
{
	/* Set AP entry point */
	Addr_t ApEntry = (Addr_t)(Addr_t*)SmpApEntry;

	/* Edit Trampoline Code */
	memcpy((char*)TrampolineCode + 0xB, &ApEntry, sizeof(ApEntry));

	/* Now copy it to memory */
	memcpy((void*)TRAMPOLINE_CODE_MEM, (char*)TrampolineCode, 0x132);
}

/* Initialize a Core */
void SmpBootCore(void *Data, int n, void *UserData)
{
	/* Get cpu structure */
	ACPI_MADT_LOCAL_APIC *Core = (ACPI_MADT_LOCAL_APIC*)Data;
	uint32_t ApicId = Core->Id;
	volatile uint32_t cpu_result = 0;
	uint32_t TimeOut = 0;

	/* Dont boot bootstrap cpu */
	if (GlbBootstrapCpuId == Core->Id)
		return;

	/* Move cpu apic id to upper 8 bits */
	printf("    * Booting Core %u", ApicId);
	ApicId <<= 24;

	/* Set destination to that cpu */
	ApicWriteLocal(APIC_ICR_HIGH, ApicId);

	/* Now send INIT IPI command (0x4500) */
	ApicWriteLocal(APIC_ICR_LOW, 0x4500);

	/* Verify startup (timeout 200 ms) */
	printf("..");
	cpu_result = ApicReadLocal(APIC_ICR_LOW);
	while ((cpu_result & 0x1000)
				&& TimeOut < 200)
	{
		StallMs(1);
		cpu_result = ApicReadLocal(APIC_ICR_LOW);
		TimeOut++;
	}
	printf("..");

	if (TimeOut == 200)
	{
		/* Failed */
		printf(" failed to boot!\n");
		return;
	}

	/* Send INIT SIPI command (0x4600) */
	ApicWriteLocal(APIC_ICR_LOW, 0x4600 | 0x5);  /* Vector 5, code is located at 0x5000 */

	/* Verify startup 
	 * It should have a timeout of 200 ms, 
	 * then resend SIPI */
	TimeOut = 0;
	cpu_result = ApicReadLocal(APIC_ICR_LOW);
	while ((cpu_result & 0x1000)
				&& TimeOut < 200)
	{
		StallMs(1);
		cpu_result = ApicReadLocal(APIC_ICR_LOW);
		TimeOut++;
	}

	if (TimeOut == 200)
	{
		/* Failed, first resend SIPI */

		/* Send INIT SIPI command (0x4600) */
		ApicWriteLocal(APIC_ICR_LOW, 0x4600 | 0x5);  /* Vector 5, code is located at 0x5000 */

		/* Verify Startup */
		TimeOut = 0;
		cpu_result = ApicReadLocal(APIC_ICR_LOW);
		while ((cpu_result & 0x1000)
			&& TimeOut < 200)
		{
			StallMs(1);
			cpu_result = ApicReadLocal(APIC_ICR_LOW);
			TimeOut++;
		}

		if (TimeOut == 200)
		{
			/* Failed */
			printf(" failed to boot!\n");
			return;
		}
	}

	printf(" booted!\n");
}

/* Go through ACPI nodes */
void _SmpSetup(void)
{
	/* Setup AP Code */
	SmpApSetup();

	/* Start each CPU */
	ListExecuteOnId(GlbAcpiNodes, SmpBootCore, ACPI_MADT_TYPE_LOCAL_APIC, NULL);
}