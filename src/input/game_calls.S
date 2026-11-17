.text
.globl _select_slot
_select_slot:
 pushl %ebx
 pushl %esi
 movl 12(%esp),%ebx
 movl $0x553fd8,%esi
 pushl 16(%esp)
 movl $0x467707,%eax
 call *%eax
 addl $4,%esp
 popl %esi
 popl %ebx
 ret
.globl _move_selected
_move_selected:
 pushl %ebp
 movl %esp,%ebp
 subl $16,%esp
 movl $0x468e13,%eax
 call *%eax
 movl $1,-8(%ebp)
 cmpw $-1,%ax
 je 1f
 movw $8,-8(%ebp)
 movw %ax,-6(%ebp)
1:
 movl $0,-4(%ebp)
 movl 8(%ebp),%eax
 movl %eax,-16(%ebp)
 movl 12(%ebp),%eax
 movl %eax,-12(%ebp)
 leal -16(%ebp),%eax
 pushl %eax
 pushl $1
 movzwl 0x553fd8,%eax
 pushl %eax
 leal -8(%ebp),%eax
 pushl %eax
 pushl $-1
 movl $0x553fdc,%ecx
 movl $0x446764,%eax
 call *%eax
 movl %ebp,%esp
 popl %ebp
 ret

.globl _route_selected
_route_selected:
 pushl %ebp
 movl %esp,%ebp
 subl $16,%esp
 movl $0x468e13,%eax
 call *%eax
 movl $1,-8(%ebp)
 cmpw $-1,%ax
 je 1f
 movw $8,-8(%ebp)
 movw %ax,-6(%ebp)
1:
 movl $0,-4(%ebp)
 pushl 12(%ebp)
 pushl 8(%ebp)
 movzwl 0x553fd8,%eax
 pushl %eax
 leal -8(%ebp),%eax
 pushl %eax
 pushl $-1
 movl $0x553fdc,%ecx
 movl $0x446764,%eax
 call *%eax
 movl %ebp,%esp
 popl %ebp
 ret

.globl _ground_selected
_ground_selected:
 pushl %ebp
 movl %esp,%ebp
 pushl 8(%ebp)
 pushl $-1
 movzwl 0x553fd8,%edx
 movl $0x553fdc,%ecx
 movl $0x446948,%eax
 call *%eax
 movl %ebp,%esp
 popl %ebp
 ret

.globl _can_control
_can_control:
 movl 4(%esp),%eax
 movl $0x43b7a4,%edx
 call *%edx
 andl $1,%eax
 ret

.globl _voice_selected
_voice_selected:
 pushl %ebx
 movl 8(%esp),%eax
 pushl %eax
 movzwl 0x553fd8,%eax
 pushl %eax
 movl $0x553fdc,%ebx
 movl $0x43e3e7,%eax
 call *%eax
 addl $8,%esp
 popl %ebx
 ret

# cdecl: options, player, slots, units, points, output, facings, pointCount.
.globl _calculate_formation
_calculate_formation:
 pushl %ebp
 movl %esp,%ebp
 pushl 32(%ebp)
 pushl 28(%ebp)
 pushl 24(%ebp)
 pushl 20(%ebp)
 pushl 16(%ebp)
 pushl 12(%ebp)
 pushl 8(%ebp)
 movl 36(%ebp),%eax
 movl $0x4481d7,%edx
 call *%edx
 movl %ebp,%esp
 popl %ebp
 ret
.globl _current_formation
_current_formation:
 movl $0x468e13,%eax
 jmp *%eax

.globl _terrain_kind
_terrain_kind:
 movl 4(%esp),%eax
 movl $0x4360e6,%edx
 call *%edx
 movzbl %al,%eax
 ret

# cdecl: options, pointCount, points. Original MOVEMENT builder.
.globl _movement_options
_movement_options:
 pushl %ebp
 movl %esp,%ebp
 pushl 16(%ebp)
 pushl 12(%ebp)
 movzwl 0x553fd8,%eax
 pushl %eax
 pushl 8(%ebp)
 pushl $-1
 movl $0x553fdc,%ecx
 movl $0x446764,%eax
 call *%eax
 movl %ebp,%esp
 popl %ebp
 ret

# cdecl actor slot, object slot, signed object identifier.
.globl _can_pickup
_can_pickup:
 pushl %ebp
 movl %esp,%ebp
 pushl 16(%ebp)
 pushl 12(%ebp)
 movl 8(%ebp),%eax
 movl $0x43b3c8,%edx
 call *%edx
 movl %ebp,%esp
 popl %ebp
 ret
.globl _pickup_selected
_pickup_selected:
 pushl %ebp
 movl %esp,%ebp
 pushl 8(%ebp)
 pushl $-1
 movzwl 0x553fd8,%edx
 movl $0x553fdc,%ecx
 movl $0x446a17,%eax
 call *%eax
 movl %ebp,%esp
 popl %ebp
 ret

.globl _sound_distance_hook
_sound_distance_hook:
 pushl %edx
 pushl %eax
 call _scaled_sound_distance
 addl $8,%esp
 ret
.globl _original_sound_distance
_original_sound_distance:
 movl 4(%esp),%eax
 movl 8(%esp),%edx
 movl $0x46bfa2,%ecx
 jmp *%ecx

# Original Alt-number handler: EBX group slot, normal game thread.
.globl _register_group
_register_group:
 pushl %ebx
 movl 8(%esp),%ebx
 movl $0x46823e,%eax
 call *%eax
 popl %ebx
 ret
.globl _group_selectable
_group_selectable:
 movl 4(%esp),%eax
 movl $0x466be9,%edx
 call *%edx
 andl $1,%eax
 ret
.globl _recall_group
_recall_group:
 movl 4(%esp),%eax
 xorl %edx,%edx
 movl $0x46835b,%ecx
 jmp *%ecx

.globl _sniff_camera_input
_sniff_camera_input:
 pushl %esi
 movl 8(%esp),%esi
 movl $0x46a80a,%eax
 call *%eax
 popl %esi
 ret

.globl _script_camera_setup_hook
_script_camera_setup_hook:
 pushl 8(%esp)
 pushl 8(%esp)
 movl $0x4a04e5,%ecx
 call *%ecx
 addl $8,%esp
 pushal
 call _publish_script_camera
 popal
 ret
.globl _legacy_camera_snif_hook
_legacy_camera_snif_hook:
 pushal
 call _allow_legacy_camera_snif
 testl %eax,%eax
 popal
 jz 1f
 pushl $0x46a80a
 ret
1:
 ret

.globl _legacy_camera_move_hook
_legacy_camera_move_hook:
 pushal
 call _allow_legacy_camera_move
 testl %eax,%eax
 popal
 jz 1f
 pushl $0x450f61
 ret
1:
 ret

.globl _move_actor
_move_actor:
 pushl %ebp
 movl %esp,%ebp
 pushl 16(%ebp)
 pushl $2
 pushl $1
 pushl 12(%ebp)
 pushl $-1
 movl 8(%ebp),%ecx
 movl $0x446764,%eax
 call *%eax
 leave
 ret
