
        .globl _SignalManager
_SignalManager:
	pushl	%ebp
	movl	%esp,%ebp
        /*-------------------------------------------------------------------
        ** Save all registers
        **-----------------------------------------------------------------*/
        pushl   %eax
        pushl   %ebx
        pushl   %ecx
        pushl   %edx
        pushl   %esi
        pushl   %edi
        pushf
        pushl   %es
        pushl   %ds
/*        pushl   %ss*/
        pushl   %fs
        pushl   %gs
        /*-----------------------------------------------------------------*/

        movl    4(%ebp), %eax
        shl     $2, %eax
        movl    _SignalTable(%eax), %ebx
        call    %ebx

        /*-------------------------------------------------------------------
        ** Restore registers
        **-----------------------------------------------------------------*/
        popl    %gs
        popl    %fs
/*        popl    %ss*/
        popl    %ds
        popl    %es
        popf
        popl    %edi
        popl    %esi
        popl    %edx
        popl    %ecx
        popl    %ebx
        popl    %eax
        /*------------------------------------------------------------------*/

        popl    %ebp
        add     $4, %esp

        ret     /* resume program */

