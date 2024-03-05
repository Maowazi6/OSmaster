#swtch(&cur->context,next->context)
.globl swtch
swtch:
	movl 4(%esp), %eax    #old
	movl 8(%esp), %edx	  #new
	
	pushl %esi
	pushl %edi
	pushl %ebx
	pushl %ebp
	
	movl %esp, (%eax)
	movl (%edx), %esp
	
	popl %ebp
	popl %ebx
	popl %edi
	popl %esi
	
	ret
	
	
	
	
	#movl 20(%esp), %eax   
    #movl %esp, (%eax)  				#esp   ->   cur->context  
	#
	#movl 24(%esp), %eax				#next->context  ->   esp
	#movl (%eax), %esp