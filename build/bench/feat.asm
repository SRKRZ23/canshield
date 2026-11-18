
build/bench/feat.o:     file format elf64-x86-64


Disassembly of section .text:

0000000000000000 <cs_state_reset>:
   0:	mov    $0x18000,%edx
   5:	xor    %esi,%esi
   7:	jmp    c <cs_state_reset+0xc>
   c:	nopl   0x0(%rax)

0000000000000010 <cs_feature_name>:
  10:	lea    0x0(%rip),%rax        # 17 <cs_feature_name+0x7>
  17:	cmp    $0x23,%edi
  1a:	ja     2a <cs_feature_name+0x1a>
  1c:	movslq %edi,%rdi
  1f:	lea    0x0(%rip),%rax        # 26 <cs_feature_name+0x16>
  26:	mov    (%rax,%rdi,8),%rax
  2a:	ret
  2b:	nopl   0x0(%rax,%rax,1)

0000000000000030 <cs_features>:
  30:	mov    (%rdx),%ecx
  32:	mov    %rdx,%r9
  35:	mov    $0x1,%edx
  3a:	cmp    $0xfff,%ecx
  40:	ja     310 <cs_features+0x2e0>
  46:	push   %rbp
  47:	mov    %ecx,%r10d
  4a:	mov    %rdi,%rax
  4d:	lea    (%r10,%r10,2),%rdi
  51:	mov    %rsp,%rbp
  54:	push   %r15
  56:	push   %r14
  58:	mov    %rsi,%r14
  5b:	mov    %rdi,%rsi
  5e:	push   %r13
  60:	shl    $0x4,%rsi
  64:	push   %r12
  66:	add    %rax,%rsi
  69:	push   %rbx
  6a:	mov    0x10(%r9),%rbx
  6e:	movzbl 0x8(%rsi),%r13d
  73:	movzbl 0x4(%r9),%ecx
  78:	mov    %rbx,-0x50(%rbp)
  7c:	lea    0x0(,%rdi,8),%rbx
  84:	lea    (%r14,%rbx,1),%r15
  88:	mov    %rbx,-0x58(%rbp)
  8c:	movzbl (%r15),%r12d
  90:	test   %r13b,%r13b
  93:	je     260 <cs_features+0x230>
  99:	cmp    %cl,0x9(%rsi)
  9c:	je     328 <cs_features+0x2f8>
  a2:	mov    $0x2,%edx
  a7:	test   %r12b,%r12b
  aa:	je     318 <cs_features+0x2e8>
  b0:	lea    (%r10,%r10,2),%rdi
  b4:	movzbl %r12b,%ebx
  b8:	mov    %rdi,%r8
  bb:	shl    $0x4,%r8
  bf:	add    %r8,%rax
  c2:	mov    0x2c(%rax),%r8d
  c6:	test   %r8d,%r8d
  c9:	je     140 <cs_features+0x110>
  cb:	mov    0x34(%rax),%eax
  ce:	mov    -0x50(%rbp),%r11
  d2:	sub    0x10(%r14,%rdi,8),%r11
  d7:	mov    %r11,%rdi
  da:	mov    %rax,-0x30(%rbp)
  de:	mov    %r8d,%r11d
  e1:	mov    %rdx,%r8
  e4:	lea    0x0(,%rdi,4),%rax
  ec:	or     $0x8,%r8
  f0:	cmp    %r11,%rax
  f3:	cmovb  %r8,%rdx
  f7:	lea    (%rdi,%rdi,1),%r8
  fb:	mov    %r8,-0x38(%rbp)
  ff:	mov    %rdx,%r8
 102:	or     $0x10,%r8
 106:	cmp    %r11,-0x38(%rbp)
 10a:	cmovb  %r8,%rdx
 10e:	add    %rdi,%rax
 111:	lea    0x0(,%r11,8),%r8
 119:	add    %rax,%rax
 11c:	mov    %rdx,%r11
 11f:	or     $0x20,%r11
 123:	cmp    %r8,%rax
 126:	mov    -0x30(%rbp),%rax
 12a:	cmovb  %r11,%rdx
 12e:	lea    (%rax,%rax,2),%r8
 132:	mov    %rdx,%rax
 135:	or     $0x40,%rax
 139:	cmp    %rdi,%r8
 13c:	cmovb  %rax,%rdx
 140:	xor    %r8d,%r8d
 143:	movzbl %cl,%edi
 146:	cmp    $0x8,%cl
 149:	movl   $0x0,-0x44(%rbp)
 150:	sete   %r8b
 154:	movl   $0x0,-0x40(%rbp)
 15b:	xor    %eax,%eax
 15d:	xor    %r11d,%r11d
 160:	movl   $0x0,-0x3c(%rbp)
 167:	mov    %edi,-0x30(%rbp)
 16a:	mov    %r14,-0x60(%rbp)
 16e:	mov    %rdx,-0x38(%rbp)
 172:	mov    %r10,-0x68(%rbp)
 176:	jmp    22d <cs_features+0x1fd>
 17b:	nopl   0x0(%rax,%rax,1)
 180:	cmp    %eax,-0x30(%rbp)
 183:	jle    249 <cs_features+0x219>
 189:	cmp    0xa(%rsi),%dl
 18c:	jb     193 <cs_features+0x163>
 18e:	cmp    %dl,0x12(%rsi)
 191:	jae    1a9 <cs_features+0x179>
 193:	lea    0x7(%rax),%ecx
 196:	mov    $0x1,%edi
 19b:	movl   $0x1,-0x3c(%rbp)
 1a2:	shl    %cl,%rdi
 1a5:	or     %rdi,-0x38(%rbp)
 1a9:	test   %r12b,%r12b
 1ac:	je     21f <cs_features+0x1ef>
 1ae:	movzbl 0x1(%r15,%rax,1),%ecx
 1b4:	movzbl %dl,%r10d
 1b8:	mov    %r10d,%r14d
 1bb:	mov    %ecx,%edi
 1bd:	sub    %ecx,%r14d
 1c0:	sub    %r10d,%ecx
 1c3:	movzbl 0x22(%rsi),%r10d
 1c8:	cmp    %dl,%dil
 1cb:	cmovb  %r14d,%ecx
 1cf:	cmp    %ecx,%r10d
 1d2:	jge    1eb <cs_features+0x1bb>
 1d4:	lea    0xf(%rax),%ecx
 1d7:	mov    $0x1,%r10d
 1dd:	movl   $0x1,-0x40(%rbp)
 1e4:	shl    %cl,%r10
 1e7:	or     %r10,-0x38(%rbp)
 1eb:	movzbl 0x1a(%rsi),%ecx
 1ef:	mov    %edx,%r10d
 1f2:	xor    %edi,%r10d
 1f5:	not    %ecx
 1f7:	test   %r10b,%cl
 1fa:	je     218 <cs_features+0x1e8>
 1fc:	lea    0x17(%rax),%ecx
 1ff:	mov    $0x1,%r10d
 205:	movl   $0x1,-0x44(%rbp)
 20c:	shl    %cl,%r10
 20f:	or     %r10,-0x38(%rbp)
 213:	nopl   0x0(%rax,%rax,1)
 218:	cmp    %dil,%dl
 21b:	cmovne %r11d,%ebx
 21f:	add    $0x1,%rax
 223:	add    $0x1,%rsi
 227:	cmp    $0x8,%rax
 22b:	je     280 <cs_features+0x250>
 22d:	movzbl 0x5(%r9,%rax,1),%edx
 233:	cmp    $0xff,%dl
 236:	je     240 <cs_features+0x210>
 238:	test   $0x1,%r8b
 23c:	cmovne %r11d,%r8d
 240:	test   %r13b,%r13b
 243:	jne    180 <cs_features+0x150>
 249:	test   %r12b,%r12b
 24c:	je     21f <cs_features+0x1ef>
 24e:	movzbl 0x1(%r15,%rax,1),%edi
 254:	jmp    218 <cs_features+0x1e8>
 256:	cs nopw 0x0(%rax,%rax,1)
 260:	movzbl %r12b,%ebx
 264:	test   %r12b,%r12b
 267:	jne    140 <cs_features+0x110>
 26d:	mov    $0x5,%edx
 272:	xor    %ebx,%ebx
 274:	xor    %r12d,%r12d
 277:	jmp    140 <cs_features+0x110>
 27c:	nopl   0x0(%rax)
 280:	mov    -0x38(%rbp),%rdx
 284:	mov    -0x44(%rbp),%ecx
 287:	movb   $0x1,(%r15)
 28b:	mov    -0x68(%rbp),%r10
 28f:	mov    -0x60(%rbp),%r14
 293:	mov    %rdx,%rax
 296:	bts    $0x1f,%rdx
 29b:	test   %r8d,%r8d
 29e:	mov    -0x50(%rbp),%rsi
 2a2:	cmove  %rax,%rdx
 2a6:	mov    %rdx,%rax
 2a9:	bts    $0x20,%rdx
 2ae:	test   %ebx,%ebx
 2b0:	mov    -0x3c(%rbp),%ebx
 2b3:	cmove  %rax,%rdx
 2b7:	mov    %rdx,%rax
 2ba:	bts    $0x21,%rdx
 2bf:	test   %ebx,%ebx
 2c1:	mov    -0x40(%rbp),%ebx
 2c4:	cmove  %rax,%rdx
 2c8:	mov    %rdx,%rax
 2cb:	bts    $0x22,%rdx
 2d0:	test   %ebx,%ebx
 2d2:	cmove  %rax,%rdx
 2d6:	mov    %rdx,%rax
 2d9:	bts    $0x23,%rdx
 2de:	test   %ecx,%ecx
 2e0:	cmove  %rax,%rdx
 2e4:	lea    (%r10,%r10,2),%rax
 2e8:	mov    %rsi,0x10(%r14,%rax,8)
 2ed:	mov    -0x58(%rbp),%rsi
 2f1:	mov    0x5(%r9),%rax
 2f5:	mov    %rax,0x1(%r14,%rsi,1)
 2fa:	mov    %rdx,%rax
 2fd:	pop    %rbx
 2fe:	pop    %r12
 300:	pop    %r13
 302:	pop    %r14
 304:	pop    %r15
 306:	pop    %rbp
 307:	ret
 308:	nopl   0x0(%rax,%rax,1)
 310:	mov    %rdx,%rax
 313:	ret
 314:	nopl   0x0(%rax)
 318:	mov    $0x6,%edx
 31d:	jmp    272 <cs_features+0x242>
 322:	nopw   0x0(%rax,%rax,1)
 328:	mov    $0x4,%edx
 32d:	test   %r12b,%r12b
 330:	je     272 <cs_features+0x242>
 336:	xor    %edx,%edx
 338:	jmp    b0 <cs_features+0x80>
 33d:	nopl   (%rax)

0000000000000340 <cs_parse_candump>:
 340:	push   %rbp
 341:	mov    %rsp,%rbp
 344:	push   %r13
 346:	push   %r12
 348:	push   %rbx
 349:	mov    %rsi,%rbx
 34c:	mov    $0x28,%esi
 351:	sub    $0x18,%rsp
 355:	mov    0x0(%rip),%r12        # 35c <cs_parse_candump+0x1c>
 35c:	mov    (%r12),%rax
 360:	mov    %rax,-0x28(%rbp)
 364:	xor    %eax,%eax
 366:	call   36b <cs_parse_candump+0x2b>
 36b:	test   %rax,%rax
 36e:	je     480 <cs_parse_candump+0x140>
 374:	mov    $0xa,%edx
 379:	lea    -0x30(%rbp),%rsi
 37d:	lea    0x1(%rax),%rdi
 381:	call   386 <cs_parse_candump+0x46>
 386:	mov    -0x30(%rbp),%rdx
 38a:	cmpb   $0x2e,(%rdx)
 38d:	jne    480 <cs_parse_candump+0x140>
 393:	lea    0x1(%rdx),%r13
 397:	mov    %r13,-0x30(%rbp)
 39b:	movzbl 0x1(%rdx),%esi
 39f:	lea    -0x30(%rsi),%edx
 3a2:	cmp    $0x9,%dl
 3a5:	ja     530 <cs_parse_candump+0x1f0>
 3ab:	xor    %ecx,%ecx
 3ad:	xor    %edx,%edx
 3af:	nop
 3b0:	cmp    $0x5,%ecx
 3b3:	jg     3c7 <cs_parse_candump+0x87>
 3b5:	sub    $0x30,%esi
 3b8:	lea    (%rdx,%rdx,4),%rdx
 3bc:	add    $0x1,%ecx
 3bf:	movsbq %sil,%rsi
 3c3:	lea    (%rsi,%rdx,2),%rdx
 3c7:	add    $0x1,%r13
 3cb:	mov    %r13,-0x30(%rbp)
 3cf:	movzbl 0x0(%r13),%esi
 3d4:	lea    -0x30(%rsi),%edi
 3d7:	cmp    $0x9,%dil
 3db:	jbe    3b0 <cs_parse_candump+0x70>
 3dd:	cmp    $0x5,%ecx
 3e0:	jg     3f7 <cs_parse_candump+0xb7>
 3e2:	nopw   0x0(%rax,%rax,1)
 3e8:	lea    (%rdx,%rdx,4),%rdx
 3ec:	add    $0x1,%ecx
 3ef:	add    %rdx,%rdx
 3f2:	cmp    $0x6,%ecx
 3f5:	jne    3e8 <cs_parse_candump+0xa8>
 3f7:	imul   $0xf4240,%rax,%rax
 3fe:	mov    %r13,%rdi
 401:	mov    $0x23,%esi
 406:	add    %rdx,%rax
 409:	mov    %rax,0x10(%rbx)
 40d:	call   412 <cs_parse_candump+0xd2>
 412:	mov    %rax,%rdi
 415:	test   %rax,%rax
 418:	je     480 <cs_parse_candump+0x140>
 41a:	cmp    %rax,%r13
 41d:	jb     431 <cs_parse_candump+0xf1>
 41f:	jmp    49b <cs_parse_candump+0x15b>
 421:	nopl   0x0(%rax)
 428:	sub    $0x1,%rax
 42c:	cmp    %r13,%rax
 42f:	je     43c <cs_parse_candump+0xfc>
 431:	cmpb   $0x20,-0x1(%rax)
 435:	jne    428 <cs_parse_candump+0xe8>
 437:	cmp    %rdi,%rax
 43a:	jae    49b <cs_parse_candump+0x15b>
 43c:	xor    %ecx,%ecx
 43e:	jmp    45d <cs_parse_candump+0x11d>
 440:	lea    -0x41(%rsi),%edx
 443:	cmp    $0x5,%edx
 446:	ja     480 <cs_parse_candump+0x140>
 448:	lea    -0x37(%rsi),%edx
 44b:	test   %edx,%edx
 44d:	js     480 <cs_parse_candump+0x140>
 44f:	shl    $0x4,%ecx
 452:	add    $0x1,%rax
 456:	or     %edx,%ecx
 458:	cmp    %rax,%rdi
 45b:	je     4a0 <cs_parse_candump+0x160>
 45d:	movsbl (%rax),%esi
 460:	lea    -0x30(%rsi),%edx
 463:	cmp    $0x9,%edx
 466:	jbe    44f <cs_parse_candump+0x10f>
 468:	lea    -0x61(%rsi),%edx
 46b:	cmp    $0x5,%edx
 46e:	ja     440 <cs_parse_candump+0x100>
 470:	lea    -0x57(%rsi),%edx
 473:	test   %edx,%edx
 475:	jns    44f <cs_parse_candump+0x10f>
 477:	nopw   0x0(%rax,%rax,1)
 480:	xor    %eax,%eax
 482:	mov    -0x28(%rbp),%rdx
 486:	sub    (%r12),%rdx
 48a:	jne    539 <cs_parse_candump+0x1f9>
 490:	add    $0x18,%rsp
 494:	pop    %rbx
 495:	pop    %r12
 497:	pop    %r13
 499:	pop    %rbp
 49a:	ret
 49b:	xor    %ecx,%ecx
 49d:	nopl   (%rax)
 4a0:	mov    %ecx,(%rbx)
 4a2:	xor    %edx,%edx
 4a4:	movq   $0x0,0x5(%rbx)
 4ac:	nopl   0x0(%rax)
 4b0:	movsbl 0x1(%rdi,%rdx,2),%ecx
 4b5:	lea    -0x30(%rcx),%eax
 4b8:	cmp    $0x9,%eax
 4bb:	jbe    4e0 <cs_parse_candump+0x1a0>
 4bd:	lea    -0x61(%rcx),%eax
 4c0:	cmp    $0x5,%eax
 4c3:	jbe    520 <cs_parse_candump+0x1e0>
 4c5:	lea    -0x41(%rcx),%eax
 4c8:	cmp    $0x5,%eax
 4cb:	ja     4d4 <cs_parse_candump+0x194>
 4cd:	lea    -0x37(%rcx),%eax
 4d0:	test   %eax,%eax
 4d2:	jns    4e0 <cs_parse_candump+0x1a0>
 4d4:	mov    %dl,0x4(%rbx)
 4d7:	mov    $0x1,%eax
 4dc:	jmp    482 <cs_parse_candump+0x142>
 4de:	xchg   %ax,%ax
 4e0:	movsbl 0x2(%rdi,%rdx,2),%esi
 4e5:	lea    -0x30(%rsi),%ecx
 4e8:	cmp    $0x9,%ecx
 4eb:	jbe    504 <cs_parse_candump+0x1c4>
 4ed:	lea    -0x61(%rsi),%ecx
 4f0:	cmp    $0x5,%ecx
 4f3:	jbe    528 <cs_parse_candump+0x1e8>
 4f5:	lea    -0x41(%rsi),%ecx
 4f8:	cmp    $0x5,%ecx
 4fb:	ja     4d4 <cs_parse_candump+0x194>
 4fd:	lea    -0x37(%rsi),%ecx
 500:	test   %ecx,%ecx
 502:	js     4d4 <cs_parse_candump+0x194>
 504:	shl    $0x4,%eax
 507:	add    %ecx,%eax
 509:	mov    %al,0x5(%rbx,%rdx,1)
 50d:	add    $0x1,%rdx
 511:	cmp    $0x8,%rdx
 515:	jne    4b0 <cs_parse_candump+0x170>
 517:	jmp    4d4 <cs_parse_candump+0x194>
 519:	nopl   0x0(%rax)
 520:	lea    -0x57(%rcx),%eax
 523:	jmp    4d0 <cs_parse_candump+0x190>
 525:	nopl   (%rax)
 528:	lea    -0x57(%rsi),%ecx
 52b:	jmp    500 <cs_parse_candump+0x1c0>
 52d:	nopl   (%rax)
 530:	xor    %edx,%edx
 532:	xor    %ecx,%ecx
 534:	jmp    3e8 <cs_parse_candump+0xa8>
 539:	call   53e <cs_parse_candump+0x1fe>
