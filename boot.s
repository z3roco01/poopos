org 0x7C00
bits 16
    jmp short start
    nop
bsOemName               DB      "poopos  "      ; 0x03

;; BPB starts here

bpbBytesPerSector       DW      512             ; 0x0B
bpbSectorsPerCluster    DB      1               ; 0x0D
bpbReservedSectors      DW      9               ; 0x0E
bpbNumFats              DB      2               ; 0x10
bpbRootEntries          DW      224             ; 0x11
bpbTotalSectors         DW      65534           ; 0x13
bpbMedia                DB      0FFH            ; 0x15
bpbSectorsPerFat        DW      254               ; 0x16
bpbSectorsPerTrack      DW      18              ; 0x18
bpbHeadsPerCylinder     DW      2               ; 0x1A
bpbHiddenSectors        DD      0               ; 0x1C
bpbTotalSectorsBig      DD      0               ; 0x20

;; BPB ends here

bsDriveNum              DB      0               ; 0x24
bsUnused                DB      0               ; 0x25
bsExtBootSignature      DB      29H             ; 0x26
bsSerialNumber          DD      0xBEEFBABE      ; 0x27
bsVolumeLabel           DB      "poopos"   ; 0x2B
bsFileSystem            DB      "FAT16   "      ; 0x36

start:
    cli ; Disable interrupts

    mov [bsDriveNum], dl  ; Value passed from the BIOS into DL should be stored in the FAT info

    ; Clear all segment registers
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax

    mov al, 'A'
    mov ah, 0x0A
    mov bh, 0x00
    mov cx, 1
    int 0x10

    in al, 0x92
    test al, 2      ; test if a20 is set and jump if it is
    jnz afterA20
    or al, 2        ; set a20 if not
    and al, 0xFE
    out 0x92, al
afterA20:
    lgdt [gdt.ptr]

    ; print B to the screen with the bios function
    mov al, 'B'
    mov ah, 0x0A
    mov bh, 0x00
    mov cx, 1
    int 0x10

    ; set the video mode to mode 13h making the bios function not work
    mov ax, 0x13
    int 0x10

    ; Set PE bits of cr0
    mov eax, cr0
    or eax, 1
    mov cr0, eax

    ; Set all segement registers to point to the GDT data entry
    mov ax, 0x10

    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Long jump to the start of protected mode
    jmp 8:pmode

bits 32
pmode:
    ; put a white pixel in the very top left, showing weve made it this far
    mov byte [0xA0000], 0x0F
end:
    jmp end


; Initial GDT thats a 1:1 map of phy mem to virt mem
gdt:
.null:
    dw 0x0000   ; Low 16 bits of limt
    dw 0x0000   ; Low 16 bits of base
    db 0x00     ; Middle 8 bits of base
    db 0x00     ; Access byte
    db 0x00     ; 4 flag bits / high 4 limit bits
    db 0x00     ; High 8 base bits
.code:
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 0x9A
    db 0xCF
    db 0x00
.data:
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 0x92
    db 0xCF
    db 0x00
.ptr:
    dw gdt.ptr - gdt
    dd gdt

times 510 - ($-$$) db 0 ; pad up to 510 bytes
dw 0xAA55 ; Marks sector as bootable
