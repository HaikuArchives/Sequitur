#
#	Atomic compare and swap functions
#

									.text

#
#	int32 compare_and_swap32(int32 *location, int32 oldValue, int32 newValue);
#

#if __x86_64
									.align	8									
									.globl compare_and_swap32
			compare_and_swap32:		pushq	%rbx			# Save these
									pushq	%rdi
									movq	12(%rsp), %rdi	# Get location
									movq	16(%rsp), %rax	# Get old value
									movq	20(%rsp), %rbx	# Get new value
									lock
									cmpxchgq %rbx, (%rdi)
									sete	%cl				# get success
									xorq	%rax, %rax
									movb	%cl, %al
									popq	%rdi
									popq	%rbx
									ret
									
									
#
#	int32 compare_and_swap64(int64 *location, int64 oldValue, int64 newValue);
#

									.align	8					
									.globl	compare_and_swap64
			compare_and_swap64:		pushq	%rbx
									pushq	%rdx
									pushq	%rdi
									movq	16(%rsp), %rdi	# Location pointer
									movq	20(%rsp), %rax	# Low dword of old value
									movq	24(%rsp), %rdx	# High dword of old value
									movq	28(%rsp), %rbx	# Low dword of new value
									movq	32(%rsp), %rcx	# High dword of new value
									lock
									cmpxchg8b (%rdi)
									sete	%cl				# get success
									xorq	%rax, %rax
									movb	%cl, %al
									pop	%rdi
									pop	%rdx
									pop	%rbx
									ret
									
									.end
#else

#
#	int32 compare_and_swap32(int32 *location, int32 oldValue, int32 newValue);
#

									.align	8									
									.globl compare_and_swap32
			compare_and_swap32:		pushl	%ebx			# Save these
									pushl	%edi
									movl	12(%esp), %edi	# Get location
									movl	16(%esp), %eax	# Get old value
									movl	20(%esp), %ebx	# Get new value
									lock
									cmpxchgl %ebx, (%edi)
									sete	%cl				# get success
									xorl	%eax, %eax
									movb	%cl, %al
									popl	%edi
									popl	%ebx
									ret
									
									
#
#	int32 compare_and_swap64(int64 *location, int64 oldValue, int64 newValue);
#

									.align	8					
									.globl	compare_and_swap64
			compare_and_swap64:		pushl	%ebx
									pushl	%edx
									pushl	%edi
									movl	16(%esp), %edi	# Location pointer
									movl	20(%esp), %eax	# Low dword of old value
									movl	24(%esp), %edx	# High dword of old value
									movl	28(%esp), %ebx	# Low dword of new value
									movl	32(%esp), %ecx	# High dword of new value
									lock
									cmpxchg8b (%edi)
									sete	%cl				# get success
									xorl	%eax, %eax
									movb	%cl, %al
									popl	%edi
									popl	%edx
									popl	%ebx
									ret
									
									.end
#endif							
