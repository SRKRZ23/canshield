
build/bench/det.o:     file format elf64-x86-64


Disassembly of section .text:

0000000000000000 <cs_detector_reset>:
   0:	mov    $0x19008,%edx
   5:	xor    %esi,%esi
   7:	jmp    c <cs_detector_reset+0xc>
   c:	nopl   0x0(%rax)

0000000000000010 <cs_detect>:
  10:	push   %rbp
  11:	mov    %rsp,%rbp
  14:	push   %r14
  16:	mov    %rdx,%r14
  19:	push   %r13
  1b:	push   %r12
  1d:	mov    %rsi,%r12
  20:	push   %rbx
  21:	mov    %rcx,%rbx
  24:	call   29 <cs_detect+0x19>
  29:	mov    %rax,(%rbx)
  2c:	mov    %rax,%rdi
  2f:	call   34 <cs_detect+0x24>
  34:	mov    0x0(%rip),%rdx        # 3b <cs_detect+0x2b>
  3b:	mov    %eax,0x8(%rbx)
  3e:	cmp    (%rdx),%eax
  40:	mov    $0xfff,%edx
  45:	setge  %r13b
  49:	mov    %r13b,0xc(%rbx)
  4d:	mov    (%r14),%eax
  50:	cmp    %edx,%eax
  52:	cmova  %rdx,%rax
  56:	movzbl 0x18000(%r12,%rax,1),%edi
  5f:	add    %edi,%edi
  61:	or     %r13d,%edi
  64:	mov    %dil,0x18000(%r12,%rax,1)
  6c:	movzbl %dil,%edi
  70:	call   75 <cs_detect+0x65>
  75:	cmp    $0x3,%eax
  78:	setg   0xd(%rbx)
  7c:	mov    0x19000(%r12),%rax
  84:	and    (%rbx),%r13b
  87:	movzbl %r13b,%r13d
  8b:	lea    (%rax,%rax,1),%rdi
  8f:	or     %r13,%rdi
  92:	mov    %rdi,0x19000(%r12)
  9a:	call   9f <cs_detect+0x8f>
  9f:	cmp    $0x1,%eax
  a2:	setg   0xe(%rbx)
  a6:	pop    %rbx
  a7:	pop    %r12
  a9:	pop    %r13
  ab:	pop    %r14
  ad:	pop    %rbp
  ae:	ret
