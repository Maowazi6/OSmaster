.text
.global _start
_start:
	pushl %ebx	 
	pushl %ecx	  
	call  main
	
	pushl  %eax
	call exit

