/* Build 471b adapters only. Not linked into the production Build 434 hook.
 * Movement optimization remains disabled until its separate path is verified.
 * These functions must be called on the engine thread by a validated dispatcher.
 */
.text
.globl _movement185
# cdecl(options, unitCount, slots, pointCount, points)
_movement185:
 pushl %ebp
 movl %esp,%ebp
 pushl %ebx
 pushl %esi
 pushl %edi
 pushl 24(%ebp)
 pushl 20(%ebp)
 pushl 16(%ebp)
 pushl 12(%ebp)
 pushl $-1
 pushl $0
 movl 8(%ebp),%ecx
 movl $0x47b31c,%eax
 call *%eax
 addl $24,%esp
 popl %edi
 popl %esi
 popl %ebx
 popl %ebp
 ret

.globl _formation185
# cdecl(options, player, slots, unitCount, points, output, facings, pointCount)
_formation185:
 pushl %ebp
 movl %esp,%ebp
 subl $8,%esp
 pushl %ebx
 pushl %esi
 pushl %edi
 pushl 16(%ebp)
 pushl 20(%ebp)
 leal -8(%ebp),%ecx
 movl $0x485d99,%eax
 call *%eax
 addl $8,%esp
 pushl 32(%ebp)
 pushl 28(%ebp)
 pushl 24(%ebp)
 pushl 20(%ebp)
 pushl 16(%ebp)
 pushl 12(%ebp)
 pushl 8(%ebp)
 leal -8(%ebp),%ecx
 movl 36(%ebp),%eax
 movl $0x47cd47,%edx
 call *%edx
 addl $28,%esp
 popl %edi
 popl %esi
 popl %ebx
 leave
 ret
.globl _can_control185
_can_control185:
 movl 4(%esp),%eax
 movl $0x482b5e,%edx
 call *%edx
 andl $1,%eax
 ret

.globl _move_actor
_move_actor:
 pushl %ebp
 movl %esp,%ebp
 pushl 16(%ebp)
 pushl $2
 pushl 8(%ebp)
 pushl $1
 pushl 12(%ebp)
 call _movement185
 addl $20,%esp
 leave
 ret

.globl _detach_selected185
_detach_selected185:
 movl 4(%esp),%eax
 movl $0x4ca568,%edx
 jmp *%edx
