/*********************IMPORTANT DRAKVUF LICENSE TERMS**********************
*                                                                         *
* DRAKVUF (C) 2014-2017 Tamas K Lengyel.                                  *
* Tamas K Lengyel is hereinafter referred to as the author.               *
* This program is free software; you may redistribute and/or modify it    *
* under the terms of the GNU General Public License as published by the   *
* Free Software Foundation; Version 2 ("GPL"), BUT ONLY WITH ALL OF THE   *
* CLARIFICATIONS AND EXCEPTIONS DESCRIBED HEREIN.  This guarantees your   *
* right to use, modify, and redistribute this software under certain      *
* conditions.  If you wish to embed DRAKVUF technology into proprietary   *
* software, alternative licenses can be aquired from the author.          *
*                                                                         *
* Note that the GPL places important restrictions on "derivative works",  *
* yet it does not provide a detailed definition of that term.  To avoid   *
* misunderstandings, we interpret that term as broadly as copyright law   *
* allows.  For example, we consider an application to constitute a        *
* derivative work for the purpose of this license if it does any of the   *
* following with any software or content covered by this license          *
* ("Covered Software"):                                                   *
*                                                                         *
* o Integrates source code from Covered Software.                         *
*                                                                         *
* o Reads or includes copyrighted data files.                             *
*                                                                         *
* o Is designed specifically to execute Covered Software and parse the    *
* results (as opposed to typical shell or execution-menu apps, which will *
* execute anything you tell them to).                                     *
*                                                                         *
* o Includes Covered Software in a proprietary executable installer.  The *
* installers produced by InstallShield are an example of this.  Including *
* DRAKVUF with other software in compressed or archival form does not     *
* trigger this provision, provided appropriate open source decompression  *
* or de-archiving software is widely available for no charge.  For the    *
* purposes of this license, an installer is considered to include Covered *
* Software even if it actually retrieves a copy of Covered Software from  *
* another source during runtime (such as by downloading it from the       *
* Internet).                                                              *
*                                                                         *
* o Links (statically or dynamically) to a library which does any of the  *
* above.                                                                  *
*                                                                         *
* o Executes a helper program, module, or script to do any of the above.  *
*                                                                         *
* This list is not exclusive, but is meant to clarify our interpretation  *
* of derived works with some common examples.  Other people may interpret *
* the plain GPL differently, so we consider this a special exception to   *
* the GPL that we apply to Covered Software.  Works which meet any of     *
* these conditions must conform to all of the terms of this license,      *
* particularly including the GPL Section 3 requirements of providing      *
* source code and allowing free redistribution of the work as a whole.    *
*                                                                         *
* Any redistribution of Covered Software, including any derived works,    *
* must obey and carry forward all of the terms of this license, including *
* obeying all GPL rules and restrictions.  For example, source code of    *
* the whole work must be provided and free redistribution must be         *
* allowed.  All GPL references to "this License", are to be treated as    *
* including the terms and conditions of this license text as well.        *
*                                                                         *
* Because this license imposes special exceptions to the GPL, Covered     *
* Work may not be combined (even as part of a larger work) with plain GPL *
* software.  The terms, conditions, and exceptions of this license must   *
* be included as well.  This license is incompatible with some other open *
* source licenses as well.  In some cases we can relicense portions of    *
* DRAKVUF or grant special permissions to use it in other open source     *
* software.  Please contact tamas.k.lengyel@gmail.com with any such       *
* requests.  Similarly, we don't incorporate incompatible open source     *
* software into Covered Software without special permission from the      *
* copyright holders.                                                      *
*                                                                         *
* If you have any questions about the licensing restrictions on using     *
* DRAKVUF in other works, are happy to help.  As mentioned above,         *
* alternative license can be requested from the author to integrate       *
* DRAKVUF into proprietary applications and appliances.  Please email     *
* tamas.k.lengyel@gmail.com for further information.                      *
*                                                                         *
* If you have received a written license agreement or contract for        *
* Covered Software stating terms other than these, you may choose to use  *
* and redistribute Covered Software under those terms instead of these.   *
*                                                                         *
* Source is provided to this software because we believe users have a     *
* right to know exactly what a program is going to do before they run it. *
* This also allows you to audit the software for security holes.          *
*                                                                         *
* Source code also allows you to port DRAKVUF to new platforms, fix bugs, *
* and add new features.  You are highly encouraged to submit your changes *
* on https://github.com/tklengyel/drakvuf, or by other methods.           *
* By sending these changes, it is understood (unless you specify          *
* otherwise) that you are offering unlimited, non-exclusive right to      *
* reuse, modify, and relicense the code.  DRAKVUF will always be          *
* available Open Source, but this is important because the inability to   *
* relicense code has caused devastating problems for other Free Software  *
* projects (such as KDE and NASM).                                        *
* To specify special license conditions of your contributions, just say   *
* so when you send them.                                                  *
*                                                                         *
* This program is distributed in the hope that it will be useful, but     *
* WITHOUT ANY WARRANTY; without even the implied warranty of              *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the DRAKVUF   *
* license file for more details (it's in a COPYING file included with     *
* DRAKVUF, and also available from                                        *
* https://github.com/tklengyel/drakvuf/COPYING)                           *
*                                                                         *
***************************************************************************/

#include <libvmi/libvmi.h>
#include <libvmi/libvmi_extra.h>
#include <libvmi/x86.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <stdio.h>
#include <signal.h>
#include <inttypes.h>
#include <glib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "libdrakvuf/libdrakvuf.h"
#include "private.h"

struct doppelganging
{
    // Inputs:
    const char* host_file;
    const char* local_proc;
    reg_t target_cr3;
    vmi_pid_t target_pid;
    uint32_t target_tid;

    // Internal:
    drakvuf_t drakvuf;
    vmi_instance_t vmi;
    const char* rekall_profile;
    bool is32bit;
    int hijacked_status;
    addr_t createprocessa;
    addr_t ntcreatesection, loadlibrary, getlasterror, createtransaction, createfiletransacted, virtualalloc, rtlzeromemory, writefile;
    addr_t eprocess_base;

    addr_t hTransaction;        // HANDLE
    addr_t hTransactedFile;     // HANDLE

    void *hostfile_buffer;
    int64_t hostfile_len;
    addr_t guestfile_buffer;
    addr_t dwBytesWritten;

    addr_t process_info;
    x86_registers_t saved_regs;

    drakvuf_trap_t bp, cr3_event;

    size_t offsets[OFFSET_MAX];

    // Results:
    reg_t cr3;
    int rc;
    uint32_t pid, tid;
    uint32_t hProc, hThr;
};

#define SW_SHOWDEFAULT 10


struct startup_info_64
{
    uint32_t cb;
    addr_t lpReserved;
    addr_t lpDesktop;
    addr_t lpTitle;
    uint32_t dwX;
    uint32_t dwY;
    uint32_t dwXSize;
    uint32_t dwYSize;
    uint32_t dwXCountChars;
    uint32_t dwYCountChars;
    uint32_t dwFillAttribute;
    uint32_t dwFlags;
    uint16_t wShowWindow;
    uint16_t cbReserved2;
    addr_t lpReserved2;
    addr_t hStdInput;
    addr_t hStdOutput;
    addr_t hStdError;
} __attribute__ ((packed));
// was not packed

struct process_information_64
{
    addr_t hProcess;
    addr_t hThread;
    uint32_t dwProcessId;
    uint32_t dwThreadId;
} __attribute__ ((packed));

struct list_entry_32
{
    uint32_t flink;
    uint32_t blink;
} __attribute__ ((packed));

struct list_entry_64
{
    uint64_t flink;
    uint64_t blink;
} __attribute__ ((packed));

struct kapc_state_64
{
    // apc_list_head[0] = kernel apc list
    // apc_list_head[1] = user apc list
    struct list_entry_64 apc_list_head[2];
    uint64_t process;
    uint8_t kernel_apc_in_progress;
    uint8_t kernel_apc_pending;
    uint8_t user_apc_pending;
} __attribute__ ((packed));
// was not packed

struct kapc_64
{
    uint8_t type;
    uint8_t spare_byte0;
    uint8_t size;
    uint8_t spare_byte1;
    uint32_t spare_long0;
    uint64_t thread;
    struct list_entry_64 apc_list_entry;
    uint64_t kernel_routine;
    uint64_t rundown_routine;
    uint64_t normal_routine;
    uint64_t normal_context;
    uint64_t system_argument_1;
    uint64_t system_argument_2;
    uint8_t apc_state_index;
    uint8_t apc_mode;
    uint8_t inserted;
} __attribute__ ((packed));
// was not packed




/*
    Create stack to call LoadLibrary

    HMODULE WINAPI LoadLibrary( _In_ LPCTSTR lpFileName );
*/
bool loadlibrary_inputs(struct doppelganging* doppelganging, drakvuf_trap_info_t* info, const char* dllname)
{
    addr_t stack_base, stack_limit;

    // get VMI
    vmi_instance_t vmi = doppelganging->vmi;

    reg_t rsp = info->regs->rsp;
    reg_t fsgs = info->regs->gs_base;

    // set Context
    access_context_t ctx =
    {
        .translate_mechanism = VMI_TM_PROCESS_DTB,
        .dtb = info->regs->cr3,
    };

    // get Stack Base
    ctx.addr = fsgs + doppelganging->offsets[NT_TIB_STACKBASE];
    if (VMI_FAILURE == vmi_read_addr(vmi, &ctx, &stack_base))
        goto err;

    // get Stack Limit
    ctx.addr = fsgs + doppelganging->offsets[NT_TIB_STACKLIMIT];
    if (VMI_FAILURE == vmi_read_addr(vmi, &ctx, &stack_limit))
        goto err;


    // Push input arguments on the stack
    uint8_t nul8 = 0;
    uint64_t nul64 = 0;
    addr_t str_addr;

    // stack start here
    addr_t addr = rsp;


    addr -= 0x8; // the stack has to be alligned to 0x8
    // and we need a bit of extra buffer before the string for \0

    // we just going to null out that extra space fully
    ctx.addr = addr;
    if (VMI_FAILURE == vmi_write_64(vmi, &ctx, &nul64))
        goto err;

    // this string has to be aligned as well
    size_t len = strlen(dllname);
    addr -= len + 0x8 - (len % 0x8);
    str_addr = addr;    // string address
    ctx.addr = addr;
    if (VMI_FAILURE == vmi_write(vmi, &ctx, len, (void*) dllname, NULL))
        goto err;
    PRINT_DEBUG("Copied string: %s (len 0x%lx) on stack\n", dllname, len);

    // add null termination
    ctx.addr = addr+len;
    if (VMI_FAILURE == vmi_write_8(vmi, &ctx, &nul8))
        goto err;


    //http://www.codemachine.com/presentations/GES2010.TRoy.Slides.pdf
    //
    //First 4 parameters to functions are always passed in registers
    //P1=rcx, P2=rdx, P3=r8, P4=r9
    //5th parameter onwards (if any) passed via the stack

    // WARNING: allocate MIN 0x10 "homing space" on stack or call will crash
    addr -= 0x8;
    ctx.addr = addr;
    if (VMI_FAILURE == vmi_write_64(vmi, &ctx, &nul64))
        goto err;

    addr -= 0x8;
    ctx.addr = addr;
    if (VMI_FAILURE == vmi_write_64(vmi, &ctx, &nul64))
        goto err;

/*
    addr -= 0x8;
    ctx.addr = addr;
    if (VMI_FAILURE == vmi_write_64(vmi, &ctx, &nul64))
        goto err;

    addr -= 0x8;
    ctx.addr = addr;
    if (VMI_FAILURE == vmi_write_64(vmi, &ctx, &nul64))
        goto err;
*/

    //p1
    info->regs->rcx = str_addr;
/*    
    //p2
    info->regs->rdx = 0;
    //p3
    info->regs->r8 = 0;
    //p4
    info->regs->r9 = 0;
*/
    
    // save the return address
    addr -= 0x8;
    ctx.addr = addr;
    if (VMI_FAILURE == vmi_write_64(vmi, &ctx, &info->regs->rip))
        goto err;

    // Grow the stack
    info->regs->rsp = addr;

    return 1;

err:
    PRINT_DEBUG("Failed to pass inputs to loadlibrary hijacked function!\n");
    return 0;
}



/*
    Create stack to call CreateTransaction

    HANDLE WINAPI CreateTransaction(
      _In_opt_ LPSECURITY_ATTRIBUTES lpTransactionAttributes,
      _In_opt_ LPGUID                UOW,
      _In_opt_ DWORD                 CreateOptions,
      _In_opt_ DWORD                 IsolationLevel,
      _In_opt_ DWORD                 IsolationFlags,
      _In_opt_ DWORD                 Timeout,
      _In_opt_ LPWSTR                Description
    );

    Example:

    HANDLE hTransaction = CreateTransaction(NULL,0,0,0,0,0,L"explorer.exe");
*/
bool createtransaction_inputs(struct doppelganging* doppelganging, drakvuf_trap_info_t* info)
{
    addr_t stack_base, stack_limit;

    // get VMI
    vmi_instance_t vmi = doppelganging->vmi;

    reg_t rsp = info->regs->rsp;
    reg_t fsgs = info->regs->gs_base;

    // set Context
    access_context_t ctx =
    {
        .translate_mechanism = VMI_TM_PROCESS_DTB,
        .dtb = info->regs->cr3,
    };

    // get Stack Base
    ctx.addr = fsgs + doppelganging->offsets[NT_TIB_STACKBASE];
    if (VMI_FAILURE == vmi_read_addr(vmi, &ctx, &stack_base))
        goto err;

    // get Stack Limit
    ctx.addr = fsgs + doppelganging->offsets[NT_TIB_STACKLIMIT];
    if (VMI_FAILURE == vmi_read_addr(vmi, &ctx, &stack_limit))
        goto err;


    // Push input arguments on the stack
    uint8_t nul8 = 0;
    uint64_t nul64 = 0;
    addr_t str_addr;

    // stack start here
    addr_t addr = rsp;


    addr -= 0x8; // the stack has to be alligned to 0x8
    // and we need a bit of extra buffer before the string for \0

    // we just going to null out that extra space fully
    ctx.addr = addr;
    if (VMI_FAILURE == vmi_write_64(vmi, &ctx, &nul64))
        goto err;

    // this string has to be aligned as well
    size_t len = strlen(doppelganging->local_proc);
    addr -= len + 0x8 - (len % 0x8);
    str_addr = addr;    // string address
    ctx.addr = addr;
    if (VMI_FAILURE == vmi_write(vmi, &ctx, len, (void*) doppelganging->local_proc, NULL))
        goto err;
    PRINT_DEBUG("Copied string: %s (len 0x%lx) on stack\n", doppelganging->local_proc, len);

    // add null termination
    ctx.addr = addr+len;
    if (VMI_FAILURE == vmi_write_8(vmi, &ctx, &nul8))
        goto err;


    //http://www.codemachine.com/presentations/GES2010.TRoy.Slides.pdf
    //
    //First 4 parameters to functions are always passed in registers
    //P1=rcx, P2=rdx, P3=r8, P4=r9
    //5th parameter onwards (if any) passed via the stack

    // p7
    // _In_opt_ LPWSTR Description 
    addr -= 0x8;
    ctx.addr = addr;
    if (VMI_FAILURE == vmi_write_64(vmi, &ctx, &str_addr))
        goto err;

    // p6
    // _In_opt_ DWORD Timeout,
    addr -= 0x8;
    ctx.addr = addr;
    if (VMI_FAILURE == vmi_write_64(vmi, &ctx, &nul64))
        goto err;

    // p5
    // _In_opt_ DWORD IsolationFlags 
    addr -= 0x8;
    ctx.addr = addr;
    if (VMI_FAILURE == vmi_write_64(vmi, &ctx, &nul64))
        goto err;



    // WARNING: allocate MIN 0x20 "homing space" on stack or call will crash
    addr -= 0x8;
    ctx.addr = addr;
    if (VMI_FAILURE == vmi_write_64(vmi, &ctx, &nul64))
        goto err;

    addr -= 0x8;
    ctx.addr = addr;
    if (VMI_FAILURE == vmi_write_64(vmi, &ctx, &nul64))
        goto err;

    addr -= 0x8;
    ctx.addr = addr;
    if (VMI_FAILURE == vmi_write_64(vmi, &ctx, &nul64))
        goto err;

    addr -= 0x8;
    ctx.addr = addr;
    if (VMI_FAILURE == vmi_write_64(vmi, &ctx, &nul64))
        goto err;


    // p1: _In_opt_ LPSECURITY_ATTRIBUTES lpTransactionAttributes
    info->regs->rcx = 0;

    // p2: _In_opt_ LPGUID UOW
    info->regs->rdx = 0;

    // p3: _In_opt_ DWORD CreateOptions 
    info->regs->r8 = 0;

    // p4: _In_opt_ DWORD IsolationLevel 
    info->regs->r9 = 0;

    
    // save the return address
    addr -= 0x8;
    ctx.addr = addr;
    if (VMI_FAILURE == vmi_write_64(vmi, &ctx, &info->regs->rip))
        goto err;

    // Grow the stack
    info->regs->rsp = addr;

    return 1;

err:
    PRINT_DEBUG("Failed to pass inputs to createtransaction hijacked function!\n");
    return 0;
}



/*
    Create stack to call CreateFileTransacted

    HANDLE WINAPI CreateFileTransacted(
      _In_       LPCTSTR               lpFileName,
      _In_       DWORD                 dwDesiredAccess,
      _In_       DWORD                 dwShareMode,
      _In_opt_   LPSECURITY_ATTRIBUTES lpSecurityAttributes,
      _In_       DWORD                 dwCreationDisposition,
      _In_       DWORD                 dwFlagsAndAttributes,
      _In_opt_   HANDLE                hTemplateFile,
      _In_       HANDLE                hTransaction,
      _In_opt_   PUSHORT               pusMiniVersion,
      _Reserved_ PVOID                 pExtendedParameter
    );

    Example:

    HANDLE hTransactedFile = CreateFileTransacted(
                                "explorer.exe", GENERIC_WRITE | GENERIC_READ, 0, NULL, 
                                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL, hTransaction, 
                                NULL, NULL);
*/
bool createfiletransacted_inputs(struct doppelganging* doppelganging, drakvuf_trap_info_t* info)
{
    addr_t stack_base, stack_limit;

    // get VMI
    vmi_instance_t vmi = doppelganging->vmi;

    reg_t rsp = info->regs->rsp;
    reg_t fsgs = info->regs->gs_base;

    // set Context
    access_context_t ctx =
    {
        .translate_mechanism = VMI_TM_PROCESS_DTB,
        .dtb = info->regs->cr3,
    };

    // get Stack Base
    ctx.addr = fsgs + doppelganging->offsets[NT_TIB_STACKBASE];
    if (VMI_FAILURE == vmi_read_addr(vmi, &ctx, &stack_base))
        goto err;

    // get Stack Limit
    ctx.addr = fsgs + doppelganging->offsets[NT_TIB_STACKLIMIT];
    if (VMI_FAILURE == vmi_read_addr(vmi, &ctx, &stack_limit))
        goto err;


    // Push input arguments on the stack
    uint8_t nul8 = 0;
    uint64_t nul64 = 0;
    addr_t str_addr;

    // stack start here
    addr_t addr = rsp;


    addr -= 0x8; // the stack has to be alligned to 0x8
    // and we need a bit of extra buffer before the string for \0

    // we just going to null out that extra space fully
    ctx.addr = addr;
    if (VMI_FAILURE == vmi_write_64(vmi, &ctx, &nul64))
        goto err;

    // this string has to be aligned as well
    size_t len = strlen(doppelganging->local_proc);
    addr -= len + 0x8 - (len % 0x8);
    str_addr = addr;    // string address
    ctx.addr = addr;
    if (VMI_FAILURE == vmi_write(vmi, &ctx, len, (void*) doppelganging->local_proc, NULL))
        goto err;
    PRINT_DEBUG("Copied string: %s (len 0x%lx) on stack @ va 0x%lx\n", doppelganging->local_proc, len, addr);

    // add null termination
    ctx.addr = addr+len;
    if (VMI_FAILURE == vmi_write_8(vmi, &ctx, &nul8))
        goto err;


    addr -= 0x8;
    ctx.addr = addr;
    if (VMI_FAILURE == vmi_write_64(vmi, &ctx, &nul64))
        goto err;



    //http://www.codemachine.com/presentations/GES2010.TRoy.Slides.pdf
    //
    //First 4 parameters to functions are always passed in registers
    //P1=rcx, P2=rdx, P3=r8, P4=r9
    //5th parameter onwards (if any) passed via the stack

    PRINT_DEBUG("CreateFileTransacted() stack:\n");

    // p10
    // _Reserved_ PVOID pExtendedParameter 
    addr -= 0x8;
    ctx.addr = addr;
    if (VMI_FAILURE == vmi_write_64(vmi, &ctx, &nul64))
        goto err;
    PRINT_DEBUG("p10: 0x%lx\n", nul64);

    // p9
    // _In_opt_ PUSHORT pusMiniVersion
    addr -= 0x8;
    ctx.addr = addr;
    if (VMI_FAILURE == vmi_write_64(vmi, &ctx, &nul64))
        goto err;
    PRINT_DEBUG("p9: 0x%lx\n", nul64);

    // p8
    // _In_ HANDLE hTransaction 
    addr -= 0x8;
    ctx.addr = addr;
    if (VMI_FAILURE == vmi_write_64(vmi, &ctx, &doppelganging->hTransaction))
        goto err;
    PRINT_DEBUG("p8: 0x%lx\n", doppelganging->hTransaction);

    // p7
    // _In_opt_ HANDLE hTemplateFile 
    addr -= 0x8;
    ctx.addr = addr;
    if (VMI_FAILURE == vmi_write_64(vmi, &ctx, &nul64))
        goto err;
    PRINT_DEBUG("p7: 0x%lx\n", nul64);

    // p6
    // _In_ DWORD dwFlagsAndAttributes
    // #define FILE_ATTRIBUTE_NORMAL 0x00000080
    uint64_t k_FILE_ATTRIBUTE_NORMAL = 0x00000080;
    addr -= 0x8;
    ctx.addr = addr;
    if (VMI_FAILURE == vmi_write_64(vmi, &ctx, &k_FILE_ATTRIBUTE_NORMAL))
        goto err;
    PRINT_DEBUG("p6: 0x%lx\n", k_FILE_ATTRIBUTE_NORMAL);

    // p5
    // _In_ DWORD dwCreationDisposition
    // #define OPEN_EXISTING 3
    uint64_t k_OPEN_EXISTING = 3;
    addr -= 0x8;
    ctx.addr = addr;
    if (VMI_FAILURE == vmi_write_64(vmi, &ctx, &k_OPEN_EXISTING))
        goto err;
    PRINT_DEBUG("p5: 0x%lx\n", k_OPEN_EXISTING);



    // WARNING: allocate MIN 0x20 "homing space" on stack or call will crash
    addr -= 0x8;
    ctx.addr = addr;
    if (VMI_FAILURE == vmi_write_64(vmi, &ctx, &nul64))
        goto err;

    addr -= 0x8;
    ctx.addr = addr;
    if (VMI_FAILURE == vmi_write_64(vmi, &ctx, &nul64))
        goto err;

    addr -= 0x8;
    ctx.addr = addr;
    if (VMI_FAILURE == vmi_write_64(vmi, &ctx, &nul64))
        goto err;

    addr -= 0x8;
    ctx.addr = addr;
    if (VMI_FAILURE == vmi_write_64(vmi, &ctx, &nul64))
        goto err;


    // p1: _In_ LPCTSTR lpFileName
    info->regs->rcx = str_addr;
    PRINT_DEBUG("p1: 0x%lx\n", str_addr);

    // p2: _In_ DWORD dwDesiredAccess
    // #define GENERIC_READ (0x80000000L)
    // #define GENERIC_WRITE (0x40000000L)
    uint64_t k_GENERIC_READ  = 0x80000000;
    uint64_t k_GENERIC_WRITE = 0x40000000;
    uint64_t k_dwDesiredAccess = k_GENERIC_READ | k_GENERIC_WRITE;
    info->regs->rdx = k_dwDesiredAccess;
    PRINT_DEBUG("p2: 0x%lx\n", k_dwDesiredAccess);

    // p3: _In_ DWORD dwShareMode 
    info->regs->r8 = 0;
    PRINT_DEBUG("p3: 0x%lx\n", nul64);

    // p4: _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes
    info->regs->r9 = 0;
    PRINT_DEBUG("p4: 0x%lx\n", nul64);

    
    // save the return address
    addr -= 0x8;
    ctx.addr = addr;
    if (VMI_FAILURE == vmi_write_64(vmi, &ctx, &info->regs->rip))
        goto err;

    // Grow the stack
    info->regs->rsp = addr;

    return 1;

err:
    PRINT_DEBUG("Failed to pass inputs to createfiletransacted hijacked function!\n");
    return 0;
}



/*
    Create stack to call VirtualAlloc

    LPVOID WINAPI VirtualAlloc(
      _In_opt_ LPVOID lpAddress,
      _In_     SIZE_T dwSize,
      _In_     DWORD  flAllocationType,
      _In_     DWORD  flProtect
    );

    Example:

    BYTE* myBuf = (BYTE*)VirtualAlloc(NULL, numbdrOfBytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
*/
bool virtualalloc_inputs(struct doppelganging* doppelganging, drakvuf_trap_info_t* info)
{
    addr_t stack_base, stack_limit;

    // get VMI
    vmi_instance_t vmi = doppelganging->vmi;

    reg_t rsp = info->regs->rsp;
    reg_t fsgs = info->regs->gs_base;

    // set Context
    access_context_t ctx =
    {
        .translate_mechanism = VMI_TM_PROCESS_DTB,
        .dtb = info->regs->cr3,
    };

    // get Stack Base
    ctx.addr = fsgs + doppelganging->offsets[NT_TIB_STACKBASE];
    if (VMI_FAILURE == vmi_read_addr(vmi, &ctx, &stack_base))
        goto err;

    // get Stack Limit
    ctx.addr = fsgs + doppelganging->offsets[NT_TIB_STACKLIMIT];
    if (VMI_FAILURE == vmi_read_addr(vmi, &ctx, &stack_limit))
        goto err;


    // Push input arguments on the stack
    uint64_t nul64 = 0;

    // stack start here
    addr_t addr = rsp;


    // the stack has to be alligned to 0x8
    addr -= 0x8;
    ctx.addr = addr;
    if (VMI_FAILURE == vmi_write_64(vmi, &ctx, &nul64))
        goto err;


    //http://www.codemachine.com/presentations/GES2010.TRoy.Slides.pdf
    //
    //First 4 parameters to functions are always passed in registers
    //P1=rcx, P2=rdx, P3=r8, P4=r9
    //5th parameter onwards (if any) passed via the stack
    PRINT_DEBUG("VirtualAlloc() stack:\n");


    // WARNING: allocate MIN 0x20 "homing space" on stack or call will crash
    addr -= 0x8;
    ctx.addr = addr;
    if (VMI_FAILURE == vmi_write_64(vmi, &ctx, &nul64))
        goto err;

    addr -= 0x8;
    ctx.addr = addr;
    if (VMI_FAILURE == vmi_write_64(vmi, &ctx, &nul64))
        goto err;

    addr -= 0x8;
    ctx.addr = addr;
    if (VMI_FAILURE == vmi_write_64(vmi, &ctx, &nul64))
        goto err;

    addr -= 0x8;
    ctx.addr = addr;
    if (VMI_FAILURE == vmi_write_64(vmi, &ctx, &nul64))
        goto err;


    // p1: _In_opt_ LPVOID lpAddress
    info->regs->rcx = 0;
    PRINT_DEBUG("p1: 0x%lx\n", info->regs->rcx);

    // p2: _In_ SIZE_T dwSize
    info->regs->rdx = doppelganging->hostfile_len;
    PRINT_DEBUG("p2: 0x%lx\n", info->regs->rdx);

    // p3: _In_ DWORD flAllocationType
    // #define MEM_COMMIT 0x1000
    // #define MEM_RESERVE 0x2000
    uint64_t k_MEM_COMMIT   = 0x1000;
    uint64_t k_MEM_RESERVE  = 0x2000;
    uint64_t k_flAllocationType = k_MEM_COMMIT | k_MEM_RESERVE;
    info->regs->r8 = k_flAllocationType;
    PRINT_DEBUG("p3: 0x%lx\n", info->regs->r8);

    // p4: _In_ DWORD flProtect
    // #define PAGE_READWRITE 0x04
    // #define PAGE_EXECUTE_READWRITE 0x40
    uint64_t k_PAGE_READWRITE  = 0x4;
    //uint64_t k_PAGE_EXECUTE_READWRITE = 0x40;
    info->regs->r9 = k_PAGE_READWRITE;
    PRINT_DEBUG("p4: 0x%lx\n", info->regs->r9);


    // save the return address
    addr -= 0x8;
    ctx.addr = addr;
    if (VMI_FAILURE == vmi_write_64(vmi, &ctx, &info->regs->rip))
        goto err;

    // Grow the stack
    info->regs->rsp = addr;

    return 1;

err:
    PRINT_DEBUG("Failed to pass inputs to createtransaction hijacked function!\n");
    return 0;
}



/*
    Create stack to call WriteFile

    BOOL WINAPI WriteFile(
      _In_        HANDLE       hFile,
      _In_        LPCVOID      lpBuffer,
      _In_        DWORD        nNumberOfBytesToWrite,
      _Out_opt_   LPDWORD      lpNumberOfBytesWritten,
      _Inout_opt_ LPOVERLAPPED lpOverlapped
    );

    Example:

    WriteFile(hTransactedFile, buffer, dwFileSize, &wrote, NULL)
*/
bool writefile_inputs(struct doppelganging* doppelganging, drakvuf_trap_info_t* info)
{
    addr_t stack_base, stack_limit;

    // get VMI
    vmi_instance_t vmi = doppelganging->vmi;

    reg_t rsp = info->regs->rsp;
    reg_t fsgs = info->regs->gs_base;

    // set Context
    access_context_t ctx =
    {
        .translate_mechanism = VMI_TM_PROCESS_DTB,
        .dtb = info->regs->cr3,
    };

    // get Stack Base
    ctx.addr = fsgs + doppelganging->offsets[NT_TIB_STACKBASE];
    if (VMI_FAILURE == vmi_read_addr(vmi, &ctx, &stack_base))
        goto err;

    // get Stack Limit
    ctx.addr = fsgs + doppelganging->offsets[NT_TIB_STACKLIMIT];
    if (VMI_FAILURE == vmi_read_addr(vmi, &ctx, &stack_limit))
        goto err;


    // Push input arguments on the stack
    uint64_t nul64 = 0;

    // stack start here
    addr_t addr = rsp;


    // the stack has to be alligned to 0x8
    addr -= 0x8;
    ctx.addr = addr;
    if (VMI_FAILURE == vmi_write_64(vmi, &ctx, &nul64))
        goto err;


    // dwBytesWritten
    addr -= 0x8;
    doppelganging->dwBytesWritten = addr;
    ctx.addr = addr;
    if (VMI_FAILURE == vmi_write_64(vmi, &ctx, &nul64))
        goto err;


    //http://www.codemachine.com/presentations/GES2010.TRoy.Slides.pdf
    //
    //First 4 parameters to functions are always passed in registers
    //P1=rcx, P2=rdx, P3=r8, P4=r9
    //5th parameter onwards (if any) passed via the stack
    PRINT_DEBUG("WriteFile() stack:\n");

    // p5 
    // _Inout_opt_ LPOVERLAPPED lpOverlapped
    addr -= 0x8;
    ctx.addr = addr;
    if (VMI_FAILURE == vmi_write_64(vmi, &ctx, &nul64))
        goto err;
    PRINT_DEBUG("p5: 0x%lx\n", nul64);


    // WARNING: allocate MIN 0x20 "homing space" on stack or call will crash
    addr -= 0x8;
    ctx.addr = addr;
    if (VMI_FAILURE == vmi_write_64(vmi, &ctx, &nul64))
        goto err;

    addr -= 0x8;
    ctx.addr = addr;
    if (VMI_FAILURE == vmi_write_64(vmi, &ctx, &nul64))
        goto err;

    addr -= 0x8;
    ctx.addr = addr;
    if (VMI_FAILURE == vmi_write_64(vmi, &ctx, &nul64))
        goto err;

    addr -= 0x8;
    ctx.addr = addr;
    if (VMI_FAILURE == vmi_write_64(vmi, &ctx, &nul64))
        goto err;


    // p1: _In_ HANDLE hFile
    info->regs->rcx = doppelganging->hTransactedFile;
    PRINT_DEBUG("p1: 0x%lx\n", info->regs->rcx);

    // p2: _In_ LPCVOID lpBuffer
    info->regs->rdx = doppelganging->guestfile_buffer;
    PRINT_DEBUG("p2: 0x%lx\n", info->regs->rdx);

    // p3: _In_ DWORD nNumberOfBytesToWrite
    info->regs->r8 = doppelganging->hostfile_len;
    PRINT_DEBUG("p3: 0x%lx\n", info->regs->r8);

    // p4: _Out_opt_ LPDWORD lpNumberOfBytesWritten
    info->regs->r9 = doppelganging->dwBytesWritten;
    PRINT_DEBUG("p4: 0x%lx\n", info->regs->r9);


    // save the return address
    addr -= 0x8;
    ctx.addr = addr;
    if (VMI_FAILURE == vmi_write_64(vmi, &ctx, &info->regs->rip))
        goto err;

    // Grow the stack
    info->regs->rsp = addr;

    return 1;

err:
    PRINT_DEBUG("Failed to pass inputs to WriteFile hijacked function!\n");
    return 0;
}




/*
    Create stack to call RtlZeroMemory

    void RtlZeroMemory(
      [in] PVOID  Destination,
      [in] SIZE_T Length
    );

*/
bool rtlzeromemory_inputs(struct doppelganging* doppelganging, drakvuf_trap_info_t* info)
{
    addr_t stack_base, stack_limit;

    // get VMI
    vmi_instance_t vmi = doppelganging->vmi;

    reg_t rsp = info->regs->rsp;
    reg_t fsgs = info->regs->gs_base;

    // set Context
    access_context_t ctx =
    {
        .translate_mechanism = VMI_TM_PROCESS_DTB,
        .dtb = info->regs->cr3,
    };

    // get Stack Base
    ctx.addr = fsgs + doppelganging->offsets[NT_TIB_STACKBASE];
    if (VMI_FAILURE == vmi_read_addr(vmi, &ctx, &stack_base))
        goto err;

    // get Stack Limit
    ctx.addr = fsgs + doppelganging->offsets[NT_TIB_STACKLIMIT];
    if (VMI_FAILURE == vmi_read_addr(vmi, &ctx, &stack_limit))
        goto err;


    // Push input arguments on the stack
    uint64_t nul64 = 0;

    // stack start here
    addr_t addr = rsp;


    // the stack has to be alligned to 0x8
    addr -= 0x8;
    ctx.addr = addr;
    if (VMI_FAILURE == vmi_write_64(vmi, &ctx, &nul64))
        goto err;


    //http://www.codemachine.com/presentations/GES2010.TRoy.Slides.pdf
    //
    //First 4 parameters to functions are always passed in registers
    //P1=rcx, P2=rdx, P3=r8, P4=r9
    //5th parameter onwards (if any) passed via the stack
    PRINT_DEBUG("RtlZeroMemory() stack:\n");


    // WARNING: allocate MIN 0x20 "homing space" on stack or call will crash
    addr -= 0x8;
    ctx.addr = addr;
    if (VMI_FAILURE == vmi_write_64(vmi, &ctx, &nul64))
        goto err;

    addr -= 0x8;
    ctx.addr = addr;
    if (VMI_FAILURE == vmi_write_64(vmi, &ctx, &nul64))
        goto err;

    addr -= 0x8;
    ctx.addr = addr;
    if (VMI_FAILURE == vmi_write_64(vmi, &ctx, &nul64))
        goto err;

    addr -= 0x8;
    ctx.addr = addr;
    if (VMI_FAILURE == vmi_write_64(vmi, &ctx, &nul64))
        goto err;


    // p1: [in] PVOID Destination
    info->regs->rcx = doppelganging->guestfile_buffer;
    PRINT_DEBUG("p1: 0x%lx\n", info->regs->rcx);

    // p2: [in] SIZE_T Length
    info->regs->rdx = doppelganging->hostfile_len;
    PRINT_DEBUG("p2: 0x%lx\n", info->regs->rdx);

    info->regs->r8 = 0;

    info->regs->r9 = 0;


    // save the return address
    addr -= 0x8;
    ctx.addr = addr;
    if (VMI_FAILURE == vmi_write_64(vmi, &ctx, &info->regs->rip))
        goto err;

    // Grow the stack
    info->regs->rsp = addr;

    return 1;

err:
    PRINT_DEBUG("Failed to pass inputs to RtlZeroMemory hijacked function!\n");
    return 0;
}





/*
    Create stack to call GetLastError

    DWORD WINAPI GetLastError(void);
*/
bool getlasterror_inputs(struct doppelganging* doppelganging, drakvuf_trap_info_t* info)
{
    addr_t stack_base, stack_limit;

    // get VMI
    vmi_instance_t vmi = doppelganging->vmi;

    reg_t rsp = info->regs->rsp;
    reg_t fsgs = info->regs->gs_base;

    // set Context
    access_context_t ctx =
    {
        .translate_mechanism = VMI_TM_PROCESS_DTB,
        .dtb = info->regs->cr3,
    };

    // get Stack Base
    ctx.addr = fsgs + doppelganging->offsets[NT_TIB_STACKBASE];
    if (VMI_FAILURE == vmi_read_addr(vmi, &ctx, &stack_base))
        goto err;

    // get Stack Limit
    ctx.addr = fsgs + doppelganging->offsets[NT_TIB_STACKLIMIT];
    if (VMI_FAILURE == vmi_read_addr(vmi, &ctx, &stack_limit))
        goto err;


    // stack start here
    addr_t addr = rsp;


    // save the return address
    addr -= 0x8;
    ctx.addr = addr;
    if (VMI_FAILURE == vmi_write_64(vmi, &ctx, &info->regs->rip))
        goto err;

    // Grow the stack
    info->regs->rsp = addr;

    return 1;

err:
    PRINT_DEBUG("Failed to pass inputs to GetLastError hijacked function!\n");
    return 0;
}



bool readhostfile(struct doppelganging* doppelganging)
{
    int hfile_fd = 0;

    doppelganging->hostfile_len = 0;
    doppelganging->hostfile_buffer = NULL;

    // open host file to inject
    hfile_fd = open(doppelganging->host_file, O_RDONLY);
    if ( hfile_fd < 0 )
    {
        PRINT_DEBUG("Failed to open host file\n");
        goto err;
    }

    struct stat hfile_info;
    if ( stat(doppelganging->host_file, &hfile_info) < 0 )
    {
        PRINT_DEBUG("Failed retrieving information about host file\n");
        goto err;
    }

    doppelganging->hostfile_len = hfile_info.st_size;
    if ( doppelganging->hostfile_len <= 0 )
    {
        PRINT_DEBUG("Error, host file size is wrong\n");
        goto err;
    }

    doppelganging->hostfile_buffer = malloc(doppelganging->hostfile_len);
    if ( ! doppelganging->hostfile_buffer )
    {
        PRINT_DEBUG("Failed to malloc host file buffer\n");
        goto err;
    }

    if ( read(hfile_fd, doppelganging->hostfile_buffer, doppelganging->hostfile_len) < doppelganging->hostfile_len )
    {
        PRINT_DEBUG("Failed to read host file\n");
        goto err;
    }
    PRINT_DEBUG("Read 0x%lx bytes from host file %s\n", doppelganging->hostfile_len, doppelganging->host_file);


    close(hfile_fd);

    return 1;

err:
    PRINT_DEBUG("Failed to read host file\n");

    if (hfile_fd)
        close(hfile_fd);

    if (doppelganging->hostfile_buffer)
        free(doppelganging->hostfile_buffer);

    return 0;
}




bool writeguestbuffer(struct doppelganging* doppelganging, drakvuf_trap_info_t* info)
{
    // get VMI
    vmi_instance_t vmi = doppelganging->vmi;

    // set Context
    access_context_t ctx =
    {
        .translate_mechanism = VMI_TM_PROCESS_DTB,
        .dtb = info->regs->cr3,
    };


    // write host file buffer to process userspace buffer
    ctx.addr = doppelganging->guestfile_buffer;
    if (VMI_FAILURE == vmi_write(vmi, &ctx, doppelganging->hostfile_len, (void*) doppelganging->hostfile_buffer, NULL))
        goto err;
    PRINT_DEBUG("Copied host buffer (len 0x%lx) to process userspace memory @ 0x%lx\n", doppelganging->hostfile_len, doppelganging->guestfile_buffer);

    return 1;

err:
    PRINT_DEBUG("Failed to write host file buffer to process userspace buffer\n");
    return 0;
}






// CR3 register callback trap
event_response_t dg_cr3_cb(drakvuf_t drakvuf, drakvuf_trap_info_t* info)
{
    // get trap data
    struct doppelganging* doppelganging = info->trap->data;

    addr_t thread = 0;
    status_t status;

    // get CR3
    reg_t cr3 = info->regs->cr3;
    PRINT_DEBUG("CR3 changed to 0x%" PRIx64 "\n", info->regs->cr3);

    // if it is not the right process, continue
    if (cr3 != doppelganging->target_cr3)
        return 0;

    // get thread
    thread = drakvuf_get_current_thread(drakvuf, info->vcpu);
    if (!thread)
    {
        PRINT_DEBUG("cr3_cb: Failed to find current thread\n");
        return 0;
    }

    // get thread id
    uint32_t threadid = 0;
    if ( !drakvuf_get_current_thread_id(doppelganging->drakvuf, info->vcpu, &threadid) || !threadid )
        return 0;

    PRINT_DEBUG("Thread @ 0x%lx. ThreadID: %u\n", thread, threadid);

    // if thread was specified as arg and it is not the right thread, continue
    if ( doppelganging->target_tid && doppelganging->target_tid != threadid)
        return 0;


    /*
     * At this point the process is still in kernel mode, so
     * we need to trap when it enters into user mode.
     *
     * For 64-bit Windows we use the trapframe approach, where we read
     * the saved RIP from the stack trap frame and trap it.
     * When this address is hit, we hijack the flow, starting a chain of commands
     * needed to doppelganging.
     * Afterwards return the registers to the original values, thus the process continues to run.
     */
    addr_t trapframe = 0;
    status = vmi_read_addr_va(doppelganging->vmi,
                              thread + doppelganging->offsets[KTHREAD_TRAPFRAME],
                              0, &trapframe);

    if (status == VMI_FAILURE || !trapframe)
    {
        PRINT_DEBUG("cr3_cb: failed to read trapframe (0x%lx)\n", trapframe);
        return 0;
    }

    status = vmi_read_addr_va(doppelganging->vmi,
                              trapframe + doppelganging->offsets[KTRAP_FRAME_RIP],
                              0, &doppelganging->bp.breakpoint.addr);

    if (status == VMI_FAILURE || !doppelganging->bp.breakpoint.addr)
    {
        PRINT_DEBUG("Failed to read RIP from trapframe or RIP is NULL!\n");
        return 0;
    }

    // reset hijacked_status
    doppelganging->hijacked_status = CALL_NONE;

    // register breakpoint trap on "Trap Frame RIP"
    doppelganging->bp.type = BREAKPOINT;
    doppelganging->bp.name = "entry";
    doppelganging->bp.cb = dg_int3_cb;
    doppelganging->bp.data = doppelganging;
    doppelganging->bp.breakpoint.lookup_type = LOOKUP_DTB;
    doppelganging->bp.breakpoint.dtb = cr3;
    doppelganging->bp.breakpoint.addr_type = ADDR_VA;

    if ( drakvuf_add_trap(drakvuf, &doppelganging->bp) )
    {
        PRINT_DEBUG("Got return address 0x%lx from trapframe and it's now trapped!\n",
                    doppelganging->bp.breakpoint.addr);

        // unsubscribe from the CR3 trap
        drakvuf_remove_trap(drakvuf, info->trap, NULL);
    }
    else
        fprintf(stderr, "Failed to trap trapframe return address\n");

    return 0;
}


// INT3 breakpoint callback
event_response_t dg_int3_cb(drakvuf_t drakvuf, drakvuf_trap_info_t* info)
{
    // get trap data
    struct doppelganging* doppelganging = info->trap->data;
    
    reg_t cr3 = info->regs->cr3;


/* 
    // set Context
    access_context_t ctx =
    {
        .translate_mechanism = VMI_TM_PROCESS_DTB,
        .dtb = cr3,
    };
*/

    PRINT_DEBUG("INT3 Callback @ 0x%lx. CR3 0x%lx (status: %d)\n",
                info->regs->rip, cr3, doppelganging->hijacked_status);

    // check breakpoint has been hit by right process
    if ( cr3 != doppelganging->target_cr3 )
    {
        PRINT_DEBUG("INT3 received but CR3 (0x%lx) doesn't match target process (0x%lx)\n",
                    cr3, doppelganging->target_cr3);
        vmi_pid_t current_pid = -1;
        vmi_dtb_to_pid(doppelganging->vmi, cr3, &current_pid);
        PRINT_DEBUG("Current CR3 (0x%lx) is PID %d on VCPU=%d\n", cr3, current_pid, info->vcpu);
        return 0;
    }

    // check current thread exists
    uint32_t threadid = 0;
    if ( !drakvuf_get_current_thread_id(doppelganging->drakvuf, info->vcpu, &threadid) || !threadid )
    {
        PRINT_DEBUG("Skip. Error retriving current TID\n");
        return 0;
    }


    // --- CHAIN #0 ---

    // check current RIP is trapframe breakpoint and check hijacked_status
    if ( doppelganging->hijacked_status == CALL_NONE && 
         info->regs->rip == doppelganging->bp.breakpoint.addr )
    {
        // save all regs (TrapFrame original status)
        memcpy(&doppelganging->saved_regs, info->regs, sizeof(x86_registers_t));

        // === start execution chain ===

        // setup stack for LoadLibrary function call
        if ( !loadlibrary_inputs(doppelganging, info, "ktmw32.dll") )
        {
            PRINT_DEBUG("Error: failed to setup stack for LoadLibrary(KtmW32.dll)\n");
            return 0;
        }
        
        // set next chain RIP: LoadLibrary
        info->regs->rip = doppelganging->loadlibrary;

        // set status to CALL_LOADLIBRARY
        doppelganging->hijacked_status = CALL_LOADLIBRARY;

        // if target thread was not defined, the current one is defined now
        if ( !doppelganging->target_tid )
        {
            doppelganging->target_tid = threadid;
            PRINT_DEBUG("Setting TID=0x%x\n", threadid);
        }

        // goto next chain: LoadLibrary
        return VMI_EVENT_RESPONSE_SET_REGISTERS;
    }

    // skip this callback in case of:
    // - hijacked_status is CALL_NONE
    // - current RIP is not trapframe breakpoint
    // - current thread is not the target one
    if ( doppelganging->hijacked_status == CALL_NONE || 
         info->regs->rip != doppelganging->bp.breakpoint.addr || 
         threadid != doppelganging->target_tid ) 
    {
        PRINT_DEBUG("Skip Check #1. Status=%d RIP=0x%lx BP=0x%lx TID=0x%x TargetTID=0x%x VCPU=%d\n", 
            doppelganging->hijacked_status, info->regs->rip, doppelganging->bp.breakpoint.addr,
            threadid, doppelganging->target_tid, info->vcpu);
        return 0;
    }


    // --- CHAIN #1 ---
    // check status is: "waiting for LoadLibrary return"
    if ( doppelganging->hijacked_status == CALL_LOADLIBRARY )
    {
        // print LoadLibraryA return code
        PRINT_DEBUG("LoadLibraryA RAX: 0x%lx\n", info->regs->rax);

        // check LoadLibraryA return: fails==NULL
        if (! info->regs->rax) {
            PRINT_DEBUG("Error: LoadLibrary(KtmW32.dll) fails\n");
            return 0;
        }

        // Library ktmw32.dll loaded. Now we can get CreateTransaction address
        doppelganging->createtransaction = drakvuf_exportsym_to_va(doppelganging->drakvuf, doppelganging->eprocess_base, "ktmw32.dll", "CreateTransaction");
        if (!doppelganging->createtransaction)
        {
            PRINT_DEBUG("Failed to get address of ktmw32.dll!CreateTransaction\n");
            return 0;
        }
        PRINT_DEBUG("--> ktmw32.dll!CreateTransaction: 0x%lx\n", doppelganging->createtransaction);

        // === start execution chain ===

        // setup stack for CreateTransaction function call
        if ( !createtransaction_inputs(doppelganging, info) )
        {
            PRINT_DEBUG("Failed to setup stack for CreateTransaction()\n");
            return 0;
        }
        
        // set next chain RIP: CreateTransaction
        info->regs->rip = doppelganging->createtransaction;

        // set status to CALL_CREATETRANSACTION
        doppelganging->hijacked_status = CALL_CREATETRANSACTION;

        // goto next chain: CreateTransaction
        return VMI_EVENT_RESPONSE_SET_REGISTERS;
    }


    // --- CHAIN #2 ---
    // check status is: "waiting for CreateTransaction return"
    if ( doppelganging->hijacked_status == CALL_CREATETRANSACTION )
    {
        // print CreateTransaction return code
        PRINT_DEBUG("CreateTransaction RAX: 0x%lx\n", info->regs->rax);

        // check CreateTransaction return: fails==INVALID_HANDLE_VALUE (-1)
        if (info->regs->rax == 0xffffffffffffffff) {
            PRINT_DEBUG("Error: CreateTransaction() fails\n");
            return 0;
        }

        // save HANDLE returned by CreateTransaction
        doppelganging->hTransaction = info->regs->rax;

        // === start execution chain ===

        // setup stack for CreateFileTransacted function call
        if ( !createfiletransacted_inputs(doppelganging, info) )
        {
            PRINT_DEBUG("Failed to setup stack for CreateFileTransacted()\n");
            return 0;
        }
        
        // set next chain RIP: CreateFileTransacted
        info->regs->rip = doppelganging->createfiletransacted;

        // set status to CALL_CREATEFILETRANSACTED
        doppelganging->hijacked_status = CALL_CREATEFILETRANSACTED;

        // goto next chain: CreateFileTransacted
        return VMI_EVENT_RESPONSE_SET_REGISTERS;
    }



    // --- CHAIN #3 ---
    // check status is: "waiting for CreateFileTransacted return"
    if ( doppelganging->hijacked_status == CALL_CREATEFILETRANSACTED )
    {
        // print CreateFileTransacted return code
        PRINT_DEBUG("CreateFileTransacted RAX: 0x%lx\n", info->regs->rax);

        // check CreateFileTransacted return: fails==INVALID_HANDLE_VALUE (-1)
        if (info->regs->rax == 0xffffffffffffffff) {
            PRINT_DEBUG("Error: CreateFileTransacted() fails\n");
            return 0;
        }

        // save HANDLE returned by CreateFileTransacted
        doppelganging->hTransactedFile = info->regs->rax;

        // === read host file ===

        if ( !readhostfile(doppelganging) )
        {
            PRINT_DEBUG("Failed to read host file\n");
            return 0;
        }


        // === start execution chain ===

        // setup stack for VirtualAlloc function call
        if ( !virtualalloc_inputs(doppelganging, info) )
        {
            PRINT_DEBUG("Failed to setup stack for VirtualAlloc()\n");
            return 0;
        }

        // set next chain RIP: VirtualAlloc
        info->regs->rip = doppelganging->virtualalloc;

        // set status to CALL_VIRTUALALLOC
        doppelganging->hijacked_status = CALL_VIRTUALALLOC;

        // goto next chain: VirtualAlloc
        return VMI_EVENT_RESPONSE_SET_REGISTERS;
    }


    // --- CHAIN #4 ---
    // check status is: "waiting for VirtualAlloc return"
    if ( doppelganging->hijacked_status == CALL_VIRTUALALLOC )
    {
        // print VirtualAlloc return code
        PRINT_DEBUG("VirtualAlloc RAX: 0x%lx\n", info->regs->rax);

        // check VirtualAlloc return: fails==NULL
        if (! info->regs->rax) {
            PRINT_DEBUG("Error: VirtualAlloc() fails\n");
            return 0;
        }

        // save address returned by VirtualAlloc
        doppelganging->guestfile_buffer = info->regs->rax;


        // === start execution chain ===

        // setup stack for RtlZeroMemory function call
        if ( !rtlzeromemory_inputs(doppelganging, info) )
        {
            PRINT_DEBUG("Failed to setup stack for RtlZeroMemory()\n");
            return 0;
        }

        // set next chain RIP: RtlZeroMemory
        info->regs->rip = doppelganging->rtlzeromemory;

        // set status to CALL_RTLZEROMEMORY
        doppelganging->hijacked_status = CALL_RTLZEROMEMORY;

        // goto next chain: RtlZeroMemory
        return VMI_EVENT_RESPONSE_SET_REGISTERS;
    }



    // --- CHAIN #5 ---
    // check status is: "waiting for RtlZeroMemory return"
    if ( doppelganging->hijacked_status == CALL_RTLZEROMEMORY )
    {
        // print RtlZeroMemory return code
        PRINT_DEBUG("RtlZeroMemory RAX: 0x%lx\n", info->regs->rax);


        // === write to guest buffer ===

        if ( !writeguestbuffer(doppelganging, info) )
        {
            PRINT_DEBUG("Failed to write to guest file buffer\n");
            return 0;
        }


        // === start execution chain ===

        // setup stack for WriteFile function call
        if ( !writefile_inputs(doppelganging, info) )
        {
            PRINT_DEBUG("Failed to setup stack for WriteFile()\n");
            return 0;
        }

        // set next chain RIP: WriteFile
        info->regs->rip = doppelganging->writefile;

        // set status to CALL_WRITEFILE
        doppelganging->hijacked_status = CALL_WRITEFILE;

        // goto next chain: WriteFile
        return VMI_EVENT_RESPONSE_SET_REGISTERS;
    }



/* Call to be used in case of fails: GetLastError()
    // --- CHAIN #FAILS ---
    // check current RIP is trapframe breakpoint and check hijacked_status
    if ( doppelganging->hijacked_status == CALL_CREATEFILETRANSACTED && 
         info->regs->rax == 0xffffffffffffffff )
    {
        // print CreateTransaction return code
        PRINT_DEBUG("CreateFileTransacted RAX: 0x%lx\n", info->regs->rax);

        // === start execution chain ===

        // setup stack for GetLastError function call
        if ( !getlasterror_inputs(doppelganging, info) )
        {
            PRINT_DEBUG("Failed to setup stack for GetLastError()\n");
            return 0;
        }
        
        // set next chain RIP: GetLastError
        info->regs->rip = doppelganging->getlasterror;

        // set status to CALL_GETLASTERROR
        doppelganging->hijacked_status = CALL_GETLASTERROR;

        // goto next chain: GetLastError
        return VMI_EVENT_RESPONSE_SET_REGISTERS;
    }
*/

    // We are now in the return path from latest call

    // remove trapframe breakpoint trap
    drakvuf_interrupt(drakvuf, -1);
    drakvuf_remove_trap(drakvuf, &doppelganging->bp, NULL);


    // print latest call return code
    PRINT_DEBUG("RAX: 0x%lx\n", info->regs->rax);


    // restore all regs and continue execution to trap frame return point
    memcpy(info->regs, &doppelganging->saved_regs, sizeof(x86_registers_t));
    return VMI_EVENT_RESPONSE_SET_REGISTERS;
}



// Doppelganging main
int doppelganging_start_app(drakvuf_t drakvuf, vmi_pid_t pid, uint32_t tid, const char* lproc, const char* hfile)
{
    struct doppelganging doppelganging = { 0 };
    doppelganging.drakvuf = drakvuf;
    doppelganging.vmi = drakvuf_lock_and_get_vmi(drakvuf);
    doppelganging.rekall_profile = drakvuf_get_rekall_profile(drakvuf);
    doppelganging.target_pid = pid;
    doppelganging.target_tid = tid;
    doppelganging.local_proc = lproc;
    doppelganging.host_file = hfile;

    doppelganging.is32bit = (vmi_get_page_mode(doppelganging.vmi, 0) == VMI_PM_IA32E) ? 0 : 1;

    // initially, only for 64bit arch
    if (doppelganging.is32bit)
    {
        PRINT_DEBUG("Unsupported arch: 32bit\n");
        goto done;        
    }

    // get DTB (CR3) of pid process
    if ( VMI_FAILURE == vmi_pid_to_dtb(doppelganging.vmi, pid, &doppelganging.target_cr3) )
    {
        PRINT_DEBUG("Unable to find target PID's DTB\n");
        goto done;
    }

    // get offsets from the Rekall profile
    unsigned int i;
    for (i = 0; i < OFFSET_MAX; i++)
    {
        if ( !drakvuf_get_struct_member_rva(doppelganging.rekall_profile, offset_names[i][0], offset_names[i][1], &doppelganging.offsets[i]))
        {
            PRINT_DEBUG("Failed to find offset for %s:%s\n", offset_names[i][0],
                        offset_names[i][1]);
        }
    }

    PRINT_DEBUG("Target PID %u with DTB 0x%lx to start '%s'\n", pid, doppelganging.target_cr3, hfile);

    // get EPROCESS
    doppelganging.eprocess_base = 0;
    if ( !drakvuf_find_process(doppelganging.drakvuf, pid, NULL, &doppelganging.eprocess_base) )
        goto done;


    // get vaddress of functions to be called

    // CreateProcessA
    doppelganging.createprocessa = drakvuf_exportsym_to_va(doppelganging.drakvuf, doppelganging.eprocess_base, "kernel32.dll", "CreateProcessA");
    if (!doppelganging.createprocessa)
    {
        PRINT_DEBUG("Failed to get address of kernel32.dll!CreateProcessA\n");
        goto done;
    }

    // NtCreateSection
    doppelganging.ntcreatesection = drakvuf_exportsym_to_va(doppelganging.drakvuf, doppelganging.eprocess_base, "ntdll.dll", "NtCreateSection");
    if (!doppelganging.ntcreatesection)
    {
        PRINT_DEBUG("Failed to get address of ntdll.dll!NtCreateSection\n");
        goto done;
    }
    PRINT_DEBUG("ntdll.dll!NtCreateSection: 0x%lx\n", doppelganging.ntcreatesection);

    // LoadLibraryA
    doppelganging.loadlibrary = drakvuf_exportsym_to_va(doppelganging.drakvuf, doppelganging.eprocess_base, "kernel32.dll", "LoadLibraryA");
    if (!doppelganging.loadlibrary)
    {
        PRINT_DEBUG("Failed to get address of kernel32.dll!LoadLibraryA\n");
        goto done;
    }
    PRINT_DEBUG("kernel32.dll!LoadLibraryA: 0x%lx\n", doppelganging.loadlibrary);

    // CreateFileTransactedA
    doppelganging.createfiletransacted = drakvuf_exportsym_to_va(doppelganging.drakvuf, doppelganging.eprocess_base, "kernel32.dll", "CreateFileTransactedA");
    if (!doppelganging.createfiletransacted)
    {
        PRINT_DEBUG("Failed to get address of kernel32.dll!CreateFileTransactedA\n");
        goto done;
    }
    PRINT_DEBUG("kernel32.dll!CreateFileTransactedA: 0x%lx\n", doppelganging.createfiletransacted);
 
    // GetLastError
    doppelganging.getlasterror = drakvuf_exportsym_to_va(doppelganging.drakvuf, doppelganging.eprocess_base, "kernel32.dll", "GetLastError");
    if (!doppelganging.getlasterror)
    {
        PRINT_DEBUG("Failed to get address of kernel32.dll!GetLastError\n");
        goto done;
    }
    PRINT_DEBUG("kernel32.dll!GetLastError: 0x%lx\n", doppelganging.getlasterror);

    // VirtualAlloc
    doppelganging.virtualalloc = drakvuf_exportsym_to_va(doppelganging.drakvuf, doppelganging.eprocess_base, "kernel32.dll", "VirtualAlloc");
    if (!doppelganging.virtualalloc)
    {
        PRINT_DEBUG("Failed to get address of kernel32.dll!VirtualAlloc\n");
        goto done;
    }
    PRINT_DEBUG("kernel32.dll!VirtualAlloc: 0x%lx\n", doppelganging.virtualalloc);

    // RtlZeroMemory
    doppelganging.rtlzeromemory = drakvuf_exportsym_to_va(doppelganging.drakvuf, doppelganging.eprocess_base, "kernel32.dll", "RtlZeroMemory");
    if (!doppelganging.rtlzeromemory)
    {
        PRINT_DEBUG("Failed to get address of kernel32.dll!RtlZeroMemory\n");
        goto done;
    }
    PRINT_DEBUG("kernel32.dll!RtlZeroMemory: 0x%lx\n", doppelganging.rtlzeromemory);

    // WriteFile
    doppelganging.writefile = drakvuf_exportsym_to_va(doppelganging.drakvuf, doppelganging.eprocess_base, "kernel32.dll", "WriteFile");
    if (!doppelganging.writefile)
    {
        PRINT_DEBUG("Failed to get address of kernel32.dll!WriteFile\n");
        goto done;
    }
    PRINT_DEBUG("kernel32.dll!WriteFile: 0x%lx\n", doppelganging.writefile);


    // register CR3 trap
    doppelganging.cr3_event.type = REGISTER;
    doppelganging.cr3_event.reg = CR3;
    doppelganging.cr3_event.cb = dg_cr3_cb;
    doppelganging.cr3_event.data = &doppelganging;
    if ( !drakvuf_add_trap(drakvuf, &doppelganging.cr3_event) )
        goto done;

    // start loop
    PRINT_DEBUG("Starting injection loop\n");
    drakvuf_loop(drakvuf);


    // return status OK
    doppelganging.rc = 1;


    // close, remove traps and release vmi
    drakvuf_pause(drakvuf);
    drakvuf_remove_trap(drakvuf, &doppelganging.cr3_event, NULL);

done:
    PRINT_DEBUG("Finished with injection. Ret: %i\n", doppelganging.rc);
    drakvuf_release_vmi(drakvuf);
    return doppelganging.rc;
}
