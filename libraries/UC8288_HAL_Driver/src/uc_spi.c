#include <uc_spi.h>
#include <uc_gpio.h>
#include <uc_sectdefs.h>


__reset_128 void spi_prefetch1()
{
    asm("nop");
}

/*********************************************************
 * Function name    :void spim_send_data_noaddr(int cmd,
                                                char *data,
                                                int datalen,
                                                int useQpi)
 * Description      :
 * Input Parameter  :
 * Outout Parameter :void
 * Return           :void
 * Author           :yjxiong
 * establish time   :2021-04-23
 *********************************************************/
void spi_send_data_noaddr(int cmd, char* data, int datalen, int useQpi);


/*********************************************************
 * Function name    :void spim_send_data_noaddr(int cmd,
                                                char *data,
                                                int datalen,
                                                int useQpi)
 * Description      :
 * Input Parameter  :
 * Outout Parameter :void
 * Return           :void
 * Author           :yjxiong
 * establish time   :2021-04-23
 *********************************************************/
__reset void spi_setup_cmd_addr(int cmd, int cmdlen, int addr, int addrlen)
{
    int cmd_reg;
    cmd_reg = cmd << (32 - cmdlen);
    *(volatile int*) (SPI_REG_SPICMD) = cmd_reg;
    *(volatile int*) (SPI_REG_SPIADR) = addr;
    *(volatile int*) (SPI_REG_SPILEN) = (cmdlen & 0x3F) | ((addrlen << 8) & 0x3F00);
}


/*********************************************************
 * Function name    :void spim_send_data_noaddr(int cmd,
                                                char *data,
                                                int datalen,
                                                int useQpi)
 * Description      :
 * Input Parameter  :
 * Outout Parameter :void
 * Return           :void
 * Author           :yjxiong
 * establish time   :2021-04-23
 *********************************************************/
__reset void spi_setup_dummy(int dummy_rd, int dummy_wr)
{
    *(volatile int*) (SPI_REG_SPIDUM) = ((dummy_wr << 16) & 0xFFFF0000) | (dummy_rd & 0xFFFF);
}


/*********************************************************
 * Function name    :void spim_send_data_noaddr(int cmd,
                                                char *data,
                                                int datalen,
                                                int useQpi)
 * Description      :
 * Input Parameter  :
 * Outout Parameter :void
 * Return           :void
 * Author           :yjxiong
 * establish time   :2021-04-23
 *********************************************************/
__reset void spi_set_datalen(int datalen)
{
    volatile int old_len;
    old_len = *(volatile int*) (SPI_REG_SPILEN);
    old_len = ((datalen << 16) & 0xFFFF0000) | (old_len & 0xFFFF);
    *(volatile int*) (SPI_REG_SPILEN) = old_len;
}


__reset_128 void spi_prefetch2()
{
    asm("nop");
}


/*********************************************************
 * Function name    :void spim_send_data_noaddr(int cmd,
                                                char *data,
                                                int datalen,
                                                int useQpi)
 * Description      :
 * Input Parameter  :
 * Outout Parameter :void
 * Return           :void
 * Author           :yjxiong
 * establish time   :2021-04-23
 *********************************************************/
__reset void spi_start_transaction(int trans_type, int csnum)
{
    *(volatile int*) (SPI_REG_STATUS) = ((1 << (csnum + 8)) & 0xF00) | ((1 << trans_type) & 0xFF);
}


/*********************************************************
 * Function name    :void spim_send_data_noaddr(int cmd,
                                                char *data,
                                                int datalen,
                                                int useQpi)
 * Description      :
 * Input Parameter  :
 * Outout Parameter :void
 * Return           :void
 * Author           :yjxiong
 * establish time   :2021-04-23
 *********************************************************/
__reset int spi_get_status()
{
    volatile int status;
    status = *(volatile int*) (SPI_REG_STATUS);
    return status;
}


/*********************************************************
 * Function name    :void spim_send_data_noaddr(int cmd,
                                                char *data,
                                                int datalen,
                                                int useQpi)
 * Description      :
 * Input Parameter  :
 * Outout Parameter :void
 * Return           :void
 * Author           :yjxiong
 * establish time   :2021-04-23
 *********************************************************/
__reset void spi_write_fifo(int* data, int datalen)
{
    volatile int num_words, i;

    num_words = (datalen >> 5) & 0x7FF;

    if ( (datalen & 0x1F) != 0)
    { num_words++; }

    for (i = 0; i < num_words; i++)
    {
        while ((((*(volatile int*) (SPI_REG_STATUS)) >> 24) & 0xFF) >= 8);
        *(volatile int*) (SPI_REG_TXFIFO) = data[i];
    }
}

__reset_128 void spi_prefetch3()
{
    asm("nop");
}

/*********************************************************
 * Function name    :void spim_send_data_noaddr(int cmd,
                                                char *data,
                                                int datalen,
                                                int useQpi)
 * Description      :
 * Input Parameter  :
 * Outout Parameter :void
 * Return           :void
 * Author           :yjxiong
 * establish time   :2021-04-23
 *********************************************************/
__reset void spi_read_fifo(int* data, int datalen)
{
    volatile int num_words;
    /* must use register for, i,j*/
    register int i, j;
    num_words = (datalen >> 5) & 0x7FF;
    if (datalen == 0)
    { return; }

    if ( (datalen & 0x1F) != 0)
    { num_words++; }
    i = 0;
    while (1)
    {
        do
        {
            j = (((*(volatile int*) (SPI_REG_STATUS)) >> 16) & 0xFF);
        } while (j == 0);
        do
        {
            data[i++] = *(volatile int*) (SPI_REG_RXFIFO);
            j--;
        } while (j);
        if (i >= num_words)
        {
            break;
        }
    }
}

/* last function in spi lib, linked to bootstrap code.
 * calling this cause cache to fill 2nd block, so we have
 * 2 blocks of code in cache */
__reset void spi_no_op()
{
    return;
}

/**
 * @brief preftch all spi code
*/
__reset void spi_prefetch()
{
    spi_prefetch1();
    spi_prefetch2();
    spi_prefetch3();
}