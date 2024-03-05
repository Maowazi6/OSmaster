#!/usr/bin/perl -w

print ".global syscall_handler\n";
print ".text\n";
print "syscall_handler:";
print "  	pushl \$0\n";
print "  	pushl %ds\n";
print "  	pushl %es\n";
print "  	pushl %fs\n";
print "  	pushl %gs\n";
print "  	pushal\n"	 ;
print "  	pushl \$0x80\n";
print "  	pushl %edx\n";
print "  	pushl %ecx\n";
print "  	pushl %ebx\n";
print "  	movl \$syscall_table,%ebx\n";
print "  	call (%ebx,%eax,4)\n";
print "  	addl \$12, %esp\n";
print "  	movl %eax, 32(%esp)\n";
print "  	jmp int_exit\n";

for(my $i = 0; $i < 256; $i++){
    print ".globl idt_vector$i\n";
    print "idt_vector$i:\n";
    if(!($i == 8 || ($i >= 10 && $i <= 14) || $i == 17)){
        print "  pushl \$0\n";
    }
	print "  	pushl %ds\n";
	print "  	pushl %es\n";
	print "  	pushl %fs\n";
	print "  	pushl %gs\n";
    print "  	pushal\n"	 ;
	print "  	movb \$0x20,%al\n";
	print "  	outb %al, \$0xa0\n";
	print "  	outb %al, \$0x20\n";
	print "  	pushl \$$i\n";
	print "  	movl \$idt_function,%eax\n";
	print "  	movl \$$i, %ebx\n";
	print "  	call (%eax,%ebx,4)\n";
    print "  	jmp int_exit\n";
}

print "\n";
print ".text\n";
print ".global int_exit\n";
print "int_exit:\n";
print "\n";

print "addl \$4,%esp\n";
print "popal\n";
print "popl %gs\n";
print "popl %fs\n";
print "popl %es\n";
print "popl %ds\n";
print "addl \$4,%esp\n";
print "iret\n";
   
print ".data\n";
print ".globl idt_vectors\n";
print "idt_vectors:\n";
for(my $i = 0; $i < 256; $i++){
    print "  .long idt_vector$i\n";
}