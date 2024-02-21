#include <stdio.h>
#include "MFRC522.h"

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>



#define PICC_TIMEOUT_5MS    0x09
#define PICC_TIMEOUT_10MS   0x13
#define PICC_TIMEOUT_15MS   0x1E

#define GPIO_CS 28

const struct device *gpio0_dev;
const struct device *spi_dev;

struct spi_config spi_cfg;
struct spi_cs_control chip;

struct spi_buf tx_buf;
struct spi_buf rx_buf;

void SPI_Init(void)
{
 spi_dev = DEVICE_DT_GET(DT_NODELABEL(spi1));
    if (!spi_dev) {
        printk("Failed to get SPI device\n");
        return;
    }

    gpio0_dev = DEVICE_DT_GET(DT_NODELABEL(gpio0)); // GPIO cihazını tanımla

    if (!gpio0_dev) {
        printk("Failed to get GPIO device\n");
        return;
    }

    // SPI yapılandırması
    spi_cfg.frequency = 4000000;
    spi_cfg.operation = SPI_WORD_SET(8) | SPI_TRANSFER_MSB |
                        SPI_MODE_CPOL | SPI_MODE_CPHA;
    spi_cfg.slave = 0;

    // CS pini için yapılandırma
    chip.gpio.port = gpio0_dev;
    chip.gpio.pin = GPIO_CS;
    chip.gpio.dt_flags = GPIO_ACTIVE_LOW;
    chip.delay = 2;

    // SPI yapılandırmasına CS pinini ekle
    spi_cfg.cs = chip;

    printk("\r\nSPI device is initialized and ready.\n");
}

uint8_t TM_MFRC522_RdReg(uint8_t ucAddress)
{
    uint8_t command = ((ucAddress << 1) & 0x7E) | 0x80;  // MFRC522 adresi ile okuma komutunu oluştur
    uint8_t tx_data[2] = {command, 0x00};  // Okunan veriyi alacak buffer
    uint8_t rx_data[2] = {0};  // Okunan veriyi alacak buffer

    tx_buf.buf = tx_data;
    tx_buf.len = 2;

    rx_buf.buf = rx_data;
    rx_buf.len = 2;

    struct spi_buf_set tx = {
        .buffers = &tx_buf,
        .count = 1
    };

    struct spi_buf_set rx = {
        .buffers = &rx_buf,
        .count = 1
    };

    if (spi_transceive(spi_dev, &spi_cfg, &tx, &rx) != 0) {
        printk("\r\nError reading MFRC522 register\r\n");
        return 0;  // Hata durumunda 0 dön
    }

    return rx_data[1];  // Okunan veriyi dön
}


void TM_MFRC522_WrReg(uint8_t ucAddress, uint8_t ucValue)
{
    uint8_t command = (ucAddress << 1) & 0x7E;  // MFRC522 adresi ile yazma komutunu oluştur

    struct spi_buf_set tx_bufs;
    struct spi_buf txb[2];

    txb[0].buf = &command;
    txb[0].len = 1;
    txb[1].buf = &ucValue;
    txb[1].len = 1;

    tx_bufs.buffers = txb;
    tx_bufs.count = 2;

    int ret = spi_write(spi_dev, &spi_cfg, &tx_bufs);
    if (ret != 0) {
        printk("\r\nSPI transfer failed with error: %d\r\n", ret);
        // Hata durumunda gerekli işlemler yapılabilir
    } else {
        // SPI iletişimi başarıyla gerçekleştirildi
    }
}





 



void TM_MFRC522_Init(void)
    {

    TM_MFRC522_Reset();

    TM_MFRC522_WrReg(MFRC522_REG_T_MODE, 0x8D);
    TM_MFRC522_WrReg(MFRC522_REG_T_PRESCALER, 0x3E);

    TM_MFRC522_WrReg(MFRC522_REG_T_RELOAD_L, 0x09);
    TM_MFRC522_WrReg(MFRC522_REG_T_RELOAD_H, 0);

    TM_MFRC522_WrReg(MFRC522_REG_TX_AUTO, 0x40);
    TM_MFRC522_WrReg(MFRC522_REG_MODE, 0x3D);

    TM_MFRC522_WrReg(MFRC522_REG_RF_CFG, 0x70);

    TM_MFRC522_SetBitMask(MFRC522_REG_DIV1_EN, 0x80);

    TM_MFRC522_AntennaOn();


    }


/* A function to initialize SPI Instance */



void TM_MFRC522_SetBitMask(uint8_t reg, uint8_t mask)
    {
    TM_MFRC522_WrReg(reg, TM_MFRC522_RdReg(reg) | mask);
    }

void TM_MFRC522_ClearBitMask(uint8_t reg, uint8_t mask)
    {
    TM_MFRC522_WrReg(reg, TM_MFRC522_RdReg(reg) & (~mask));
    }

void TM_MFRC522_AntennaOn(void)
    {
    uint8_t temp;

    temp = TM_MFRC522_RdReg(MFRC522_REG_TX_CONTROL);
    if (!(temp & 0x03))
	TM_MFRC522_SetBitMask(MFRC522_REG_TX_CONTROL, 0x03);
    }

void TM_MFRC522_AntennaOff(void)
    {
    TM_MFRC522_ClearBitMask(MFRC522_REG_TX_CONTROL, 0x03);
    }

void TM_MFRC522_Reset(void)
    {
    uint8_t temp;

    TM_MFRC522_WrReg(MFRC522_REG_COMMAND, PCD_RESETPHASE);

    do
	{
	temp = TM_MFRC522_RdReg(MFRC522_REG_COMMAND);
	temp &= 0x10;
	}
    while (temp);
    }

uint8_t MFRC522_ToCard(uint8_t command, uint8_t *sendData, uint8_t sendLen, uint8_t *backData,unsigned *backLen)
    {
    uint8_t _status = MI_ERR;
    uint8_t irqEn = 0x00;
    uint8_t waitIRq = 0x00;
    uint8_t lastBits;
    uint8_t n;
    unsigned i;

    switch (command)
	{
    case PCD_AUTHENT:
	{
	irqEn = 0x12;
	waitIRq = 0x10;
	break;
	}
    case PCD_TRANSCEIVE:
	{
	irqEn = 0x77;
	waitIRq = 0x30;
	break;
	}
    default:
	break;
	}

    TM_MFRC522_WrReg(MFRC522_REG_COMM_IE_N, irqEn | 0x80);
    TM_MFRC522_ClearBitMask(MFRC522_REG_COMM_IRQ, 0x80);
    TM_MFRC522_SetBitMask(MFRC522_REG_FIFO_LEVEL, 0x80);
    TM_MFRC522_WrReg(MFRC522_REG_COMMAND, PCD_IDLE);

    for (i = 0; i < sendLen; i++)
	{
	TM_MFRC522_WrReg(MFRC522_REG_FIFO_DATA, sendData[i]);
	}

    TM_MFRC522_WrReg(MFRC522_REG_COMMAND, command);

    if (command == PCD_TRANSCEIVE)
	{
	TM_MFRC522_SetBitMask(MFRC522_REG_BIT_FRAMING, 0x80);
	}

    i = 0xFFFF;
    do
	{

	n = TM_MFRC522_RdReg(MFRC522_REG_COMM_IRQ);
	i--;
	}
    while (i && !(n & 0x01) && !(n & waitIRq));

    TM_MFRC522_ClearBitMask(MFRC522_REG_BIT_FRAMING, 0x80);

    if (i != 0)
	{
	if (!(TM_MFRC522_RdReg(MFRC522_REG_ERROR) & 0x1B))
	    {
	    _status = MI_OK;
	    if (n & irqEn & 0x01)
		{
		_status = MI_NOTAGERR;
		}

	    if (command == PCD_TRANSCEIVE)
		{
		n = TM_MFRC522_RdReg(MFRC522_REG_FIFO_LEVEL);
		lastBits = TM_MFRC522_RdReg(MFRC522_REG_CONTROL) & 0x07;
		if (lastBits)
		    {
		    *backLen = (n - 1) * 8 + lastBits;
		    }
		else
		    {
		    *backLen = n * 8;
		    }

		if (n == 0)
		    {
		    n = 1;
		    }

		if (n > 16)
		    {
		    n = 16;
		    }

		for (i = 0; i < n; i++)
		    {
		    backData[i] = TM_MFRC522_RdReg(MFRC522_REG_FIFO_DATA);
		    }

		backData[i] = 0;
		}
	    }
	else
	    {
	    _status = MI_ERR;
	    }

	}

    return _status;
    }

uint8_t MFRC522_Request(uint8_t reqMode, uint8_t *TagType)
    {
    uint8_t _status;
    unsigned backBits;

    TM_MFRC522_WrReg(MFRC522_REG_BIT_FRAMING, 0x07);

    TagType[0] = reqMode;
    _status = MFRC522_ToCard(PCD_TRANSCEIVE, TagType, 1, TagType, &backBits);

    if ((_status != MI_OK) || (backBits != 0x10))
	{
	_status = MI_ERR;
	}

    return _status;
    }

uint8_t MFRC522_isCard(uint8_t *TagType)
    {
    if (MFRC522_Request(PICC_REQA, TagType) == MI_OK)

	return 1;
    else
	return 0;
    }
		
TM_MFRC522_STS_T TM_MFRC522_Anticoll(uint8_t* serNum)
{
    TM_MFRC522_STS_T status;
    uint8_t i;
    uint8_t serNumCheck = 0;
    uint16_t unLen;
	
		TM_MFRC522_ClearBitMask(MFRC522_REG_STATUS2,0x08);
    TM_MFRC522_WrReg(MFRC522_REG_BIT_FRAMING, 0x00);		//TxLastBists = BitFramingReg[2..0]
		TM_MFRC522_SetBitMask(MFRC522_REG_COLL,0x80);
	
    /* ANTICOLLISION command with NVB does not specify 40 valid bits (NVB not to 0x70) */
    /* Note: "NVB=0x20" defines that the PCD will transmit no part of UID CLn, and it forces all
             PICCs in the field to respond with their complete UID CLn. */
    serNum[0] = PICC_ANTICOLL;
    serNum[1] = 0x20;   // NVB
    status = TM_MFRC522_ToCard(PCD_TRANSCEIVE, serNum, 2, serNum, &unLen);

    if (status == MI_OK) {
        /* Check card serial number */
        for (i = 0; i < 4; i++) { 
					serNumCheck ^= serNum[i];					
        }
        
        /* BCC: UID CLn checkbyte and is calculated
           as exclusive-or over the 4 previous bytes */
        if (serNumCheck != serNum[i] ) {
            status = MI_ERR;    
        }
    }
		TM_MFRC522_SetBitMask(MFRC522_REG_COLL,0x80);
		
    return status;
} 

TM_MFRC522_STS_T TM_MFRC522_ToCard(uint8_t command, uint8_t* sendData, uint8_t sendLen, uint8_t* backData, uint16_t* backLen)
{
    TM_MFRC522_STS_T status = MI_ERR;
    uint8_t irqEn = 0x00;
		uint8_t waitFor = 0x00;
    uint8_t lastBits;
    uint8_t regValue;
    uint16_t i;
    
    switch (command)
    {
        case PCD_AUTHENT:
            irqEn = 0x13;
						waitFor = 0x10;
        //    irqEn = 0x12; (original setting)
            break;
        case PCD_TRANSCEIVE:
            /* TxIRq(b6), RxIRq(b5), IdleIRq(b4),
               HiAlerIRq(b3), LoAlertIRq(b2), ErrIRq(b1), TimerIRq(b0) */
            /* wait response from PICC or 5 ms timeout */
            irqEn = 0x21;
						waitFor = 0x30;
            break;
        default:
            break;
    }
    /* IRQ pin is inverted with respect to the Status1Reg register’s IRq bit */
    TM_MFRC522_WrReg(MFRC522_REG_COMM_IE_N, irqEn | 0x80);

    /* Clear marked bits in ComIrqReg register */
    TM_MFRC522_ClearBitMask(MFRC522_REG_COMM_IRQ, 0x80);
		
		TM_MFRC522_WrReg(MFRC522_REG_COMMAND,PCD_IDLE);

    /* Flush FIFO contents */
    TM_MFRC522_SetBitMask(MFRC522_REG_FIFO_LEVEL, 0x80);

    /* Cancel current comand execution */
    TM_MFRC522_WrReg(MFRC522_REG_COMMAND, PCD_IDLE);

    /* Write data to FIFO */
    for (i = 0; i < sendLen; i++) {   
        TM_MFRC522_WrReg(MFRC522_REG_FIFO_DATA, sendData[i]);    
    }

    /* Execute PCD_TRANSCEIVE command */
    TM_MFRC522_WrReg(MFRC522_REG_COMMAND, command);
    
    if (command == PCD_TRANSCEIVE) {    
        /* Start frame transmission */
        TM_MFRC522_SetBitMask(MFRC522_REG_BIT_FRAMING, 0x80);
    }   
			
		i = 1000;
    /* Wait response from PICC or internal timer 5 ms timeout happened */
    do
		{
			
    regValue = TM_MFRC522_RdReg(MFRC522_REG_COMM_IRQ);
		i--;
		}while ((i!=0) && !(regValue&0x01) && !(regValue&waitFor));
		
		
    /* Set StartSend=0 */
    TM_MFRC522_ClearBitMask(MFRC522_REG_BIT_FRAMING, 0x80);

    /* Check if internal timer timeout */
    if (!(regValue &0x01))
    {
        if(i!=0)
				{
        if (!(TM_MFRC522_RdReg(MFRC522_REG_ERROR) & 0x1B))
        {
            status = MI_OK;
            
            if (command == PCD_TRANSCEIVE)
            {
                /* Check the number of bytes stored in FIFO */
                regValue = TM_MFRC522_RdReg(MFRC522_REG_FIFO_LEVEL);
                
                /* Check valid bit number of last byte */
                lastBits = TM_MFRC522_RdReg(MFRC522_REG_CONTROL) & 0x07;
                if (lastBits)
                    *backLen = (regValue - 1) * 8 + lastBits;   
                else
                    *backLen = regValue * 8;

                if (regValue == 0)
                    regValue = 1;    
                if (regValue > MFRC522_MAX_LEN)
                    regValue = MFRC522_MAX_LEN;   

                /* Read received data in FIFO */
                for (i = 0; i < regValue; i++)
                    backData[i] = TM_MFRC522_RdReg(MFRC522_REG_FIFO_DATA);    
            }
        }
        else
				{status = MI_ERR;}
    }

}
		TM_MFRC522_SetBitMask(MFRC522_REG_CONTROL,0x80);
		TM_MFRC522_WrReg(MFRC522_REG_COMMAND,PCD_IDLE);
    return status;
		}


		
		
		
		

void TM_MFRC522_CalculateCRC(uint8_t *pIndata, uint8_t len, uint8_t *pOutData)
    {
    uint8_t i, n;

    TM_MFRC522_ClearBitMask(MFRC522_REG_DIV_IRQ, 0x04);

    TM_MFRC522_SetBitMask(MFRC522_REG_FIFO_LEVEL, 0x80);

    for (i = 0; i < len; i++)
	{
	TM_MFRC522_WrReg(MFRC522_REG_FIFO_DATA, *(pIndata + i));
	}
    TM_MFRC522_WrReg(MFRC522_REG_COMMAND, PCD_CALCCRC);

    i = 0xFF;
    do
	{
	n = TM_MFRC522_RdReg(MFRC522_REG_DIV_IRQ);
	i--;
	}
    while ((i != 0) && !(n & 0x04));

    pOutData[0] = TM_MFRC522_RdReg(MFRC522_REG_CRC_RESULT_L);
    pOutData[1] = TM_MFRC522_RdReg(MFRC522_REG_CRC_RESULT_M);
    }


void TM_MFRC522_Halt(void)
    {

    unsigned unLen;
    uint8_t buff[4];

    buff[0] = PICC_HALT;
    buff[1] = 0;
    TM_MFRC522_CalculateCRC(buff, 2, &buff[2]);

    if (MFRC522_ToCard(PCD_TRANSCEIVE, buff, 4, buff, &unLen) != MI_NOTAGERR)
	{
	printf("Halt command error. \n");
	}

    }

		
		uint8_t TM_MFRC522_Check(uint8_t *id)
    {
    uint8_t status;
    uint8_t cardtype[3];

    status = MFRC522_Request(PICC_REQA, cardtype);
    if (status == MI_OK)
	status = TM_MFRC522_Anticoll(id);
    else
	status = MI_ERR;
		
    return status;
		
    }
		
		
		
