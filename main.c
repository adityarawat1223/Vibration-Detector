#include <stdint.h>

#define RCC_BASE 0x40021000UL
#define GPIOB_BASE 0x40010C00UL
#define I2C2_BASE 0x40005800UL
#define AFIO_BASE 0x40010000UL
#define EXTI_BASE 0x40010400UL

#define EXTI_IMR        (*((volatile uint32_t *)(EXTI_BASE + 0x00)))
#define EXTI_RTSR       (*((volatile uint32_t *)(EXTI_BASE + 0x08)))
#define EXTI_FTSR       (*((volatile uint32_t *)(EXTI_BASE + 0x0C)))
#define EXTI_PR         (*((volatile uint32_t *)(EXTI_BASE + 0x14)))

#define NVIC_ISER0      (*((volatile uint32_t *)0xE000E100UL))

#define RCC_APB2ENR (*((volatile uint32_t *)(RCC_BASE + 0x18)))
#define RCC_APB1ENR (*((volatile uint32_t *)(RCC_BASE + 0x1C)))
#define GPIOB_CRH (*((volatile uint32_t *)(GPIOB_BASE + 0x04)))
#define AFIO_EXTICR1 (*((volatile uint32_t *)(AFIO_BASE + 0x08)))

#define I2C2_CR1 (*((volatile uint32_t *)(I2C2_BASE + 0x00)))
#define I2C2_CR2 (*((volatile uint32_t *)(I2C2_BASE + 0x04)))
#define I2C2_DR (*((volatile uint32_t *)(I2C2_BASE + 0x10)))
#define I2C2_SR1 (*((volatile uint32_t *)(I2C2_BASE + 0x14)))
#define I2C2_SR2 (*((volatile uint32_t *)(I2C2_BASE + 0x18)))
#define I2C2_CCR (*((volatile uint32_t *)(I2C2_BASE + 0x1C)))
#define I2C2_TRISE (*((volatile uint32_t *)(I2C2_BASE + 0x20)))

#define MPU_ADDR (0x68 << 1)

int16_t accel_x, accel_y, accel_z;

uint8_t data[6];
volatile uint32_t temp;
volatile uint8_t read = 0;

void clock_init(void)
{
    RCC_APB2ENR |= (1 << 0);
    RCC_APB2ENR |= (1 << 3);
    RCC_APB1ENR |= (1 << 22);
}

void ExternalInterruptInit(void)
{
    AFIO_EXTICR1 &= ~(0xF << 0);
    AFIO_EXTICR1 |=  (0x1 << 0); 

    EXTI_IMR |= (1 << 0);

    EXTI_RTSR |= (1 << 0);
    EXTI_FTSR &= ~(1 << 0);

    EXTI_PR = (1 << 0);

    NVIC_ISER0 = (1 << 6);
}

void EXTI0_IRQHandler(void)
{
    if (EXTI_PR & (1 << 0))
    {
        EXTI_PR = (1 << 0);  
        read++;
    }
}

void I2Csetup(void)
{
    GPIOB_CRH &= ~(0xFF << 8);
    GPIOB_CRH |= (0xEE << 8);

    I2C2_CR2 |= 8;
    I2C2_CCR |= 40;
    I2C2_TRISE |= 9;
    I2C2_CR1 |= (1 << 0);
}

void I2C_Stop(void)
{
    I2C2_CR1 |= (1 << 9);
}

void ReadMpu(void)
{
    I2C2_CR1 |= (1 << 8);
    while (!(I2C2_SR1 & 1));

    I2C2_DR = MPU_ADDR;
    while (!(I2C2_SR1 & (1 << 1)));

    temp = I2C2_SR1;
    temp = I2C2_SR2;
    (void)temp;

    I2C2_DR = 0x3B;
    while (!(I2C2_SR1 & (1 << 2)));

    I2C2_CR1 |= (1 << 8);
    while (!(I2C2_SR1 & (1 << 0)));
    I2C2_DR = MPU_ADDR | 1;          

    while (!(I2C2_SR1 & (1 << 1)));

    I2C2_CR1 |= (1 << 10);

    temp = I2C2_SR1;
    temp = I2C2_SR2;
    (void)temp;

    while (!(I2C2_SR1 & (1 << 6)));
    data[0] = (uint8_t)I2C2_DR;

    while (!(I2C2_SR1 & (1 << 6)));
    data[1] = (uint8_t)I2C2_DR;

    while (!(I2C2_SR1 & (1 << 6)));
    data[2] = (uint8_t)I2C2_DR;

    while (!(I2C2_SR1 & (1 << 6)));
    data[3] = (uint8_t)I2C2_DR;

    while (!(I2C2_SR1 & (1 << 6)));
    I2C2_CR1 &= ~(1 << 10); 
    I2C_Stop();
    data[4] = (uint8_t)I2C2_DR;

    while (!(I2C2_SR1 & (1 << 6)));
    data[5] = (uint8_t)I2C2_DR;


    accel_x = (int16_t)((data[0] << 8) | data[1]);
    accel_y = (int16_t)((data[2] << 8) | data[3]);
    accel_z = (int16_t)((data[4] << 8) | data[5]);
}

int main(void)
{
    clock_init();
    I2Csetup();
    ExternalInterruptInit();

    while (1)
    {
        if (read)
        {
            read--;
            ReadMpu();
        }
    }
}