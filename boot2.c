__attribute__((section(".first"))) void boot2() {
    // set the 2nd pixel to a greenish colour to show we made it
    ((char *)0xA0000)[1] = 0x32;

    __asm__("jmp .");
}
