ENTRY(boot2)
OUTPUT_FORMAT("binary")
SECTIONS {
    . = 0x7E00;
    .first : {
        *(.first)
    }
    .text : {
        *(.text)
    }
    .rodata : {
        *(.rodata)
    }

    .data : {
        *(.data)
    }

    .bss : {
        *(.bss)
    }
}
