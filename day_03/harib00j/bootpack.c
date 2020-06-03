void io_hlt(void);  // prototype declaration

void HariMain(void)
{

fin:
    io_hlt();
    goto fin;

}
