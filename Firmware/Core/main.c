#include <stdint.h>
#include <math.h>

#define FFT_N 128
#define PI 3.14159265358979323846f

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
#define GPIOB_CRL (*((volatile uint32_t *)(GPIOB_BASE + 0x00)))
#define GPIOB_CRH (*((volatile uint32_t *)(GPIOB_BASE + 0x04)))
#define GPIOB_ODR (*((volatile uint32_t *)(GPIOB_BASE + 0x0C)))
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
float Acceleration[FFT_N];
float Acceleration_img[FFT_N];
float Magnitudes[FFT_N / 2];

uint16_t sample_count = 0;
uint8_t fft_done = 0;

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
    GPIOB_CRL &= ~(0xF << 0);
    GPIOB_CRL |=  (0x8 << 0);
    GPIOB_ODR &= ~(1 << 0);

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

void I2C_Write(uint8_t reg, uint8_t value)
{
    I2C2_CR1 |= (1 << 8);
    while (!(I2C2_SR1 & (1 << 0)));

    I2C2_DR = MPU_ADDR;
    while (!(I2C2_SR1 & (1 << 1)));

    temp = I2C2_SR1;
    temp = I2C2_SR2;
    (void)temp;

    I2C2_DR = reg;
    while (!(I2C2_SR1 & (1 << 7)));

    I2C2_DR = value;
    while (!(I2C2_SR1 & (1 << 2)));

    I2C_Stop();
    while (I2C2_CR1 & (1 << 9));
}

void MpuInit(void)
{
    for (volatile int i = 0; i < 100000; i++);
    I2C_Write(0x6B, 0x00);
    I2C_Write(0x1A, 0x03);
    I2C_Write(0x19, 0x07);
    I2C_Write(0x1C, 0x00);
    I2C_Write(0x37, 0x30);
    I2C_Write(0x38, 0x01);
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

    while (I2C2_CR1 & (1 << 9));

    accel_x = (int16_t)((data[0] << 8) | data[1]);
    accel_y = (int16_t)((data[2] << 8) | data[3]);
    accel_z = (int16_t)((data[4] << 8) | data[5]);
}

void RemoveOffset(void)
{
    float sum = 0.0f;
    for (uint16_t i = 0; i < FFT_N; i++)
    {
        sum += Acceleration[i];
    }
    float mean = sum / (float)FFT_N;
    for (uint16_t i = 0; i < FFT_N; i++)
    {
        Acceleration[i] -= mean;
    }
}

void FFT(float *real, float *img, uint16_t n)
{
    uint16_t j = 0;
    for (uint16_t i = 0; i < n - 1; i++)
    {
        if (i < j)
        {
            float temp_r = real[i];
            real[i] = real[j];
            real[j] = temp_r;

            float temp_i = img[i];
            img[i] = img[j];
            img[j] = temp_i;
        }
        uint16_t k = n >> 1;
        while (k <= j)
        {
            j -= k;
            k >>= 1;
        }
        j += k;
    }

    for (uint16_t len = 2; len <= n; len <<= 1)
    {
        float angle = -2.0f * PI / len;
        float wlen_r = cosf(angle);
        float wlen_i = sinf(angle);

        for (uint16_t i = 0; i < n; i += len)
        {
            float w_r = 1.0f;
            float w_i = 0.0f;

            for (uint16_t k = 0; k < len / 2; k++)
            {
                uint16_t u_idx = i + k;
                uint16_t v_idx = i + k + len / 2;

                float u_r = real[u_idx];
                float u_i = img[u_idx];

                float v_r = real[v_idx] * w_r - img[v_idx] * w_i;
                float v_i = real[v_idx] * w_i + img[v_idx] * w_r;

                real[u_idx] = u_r + v_r;
                img[u_idx] = u_i + v_i;
                real[v_idx] = u_r - v_r;
                img[v_idx] = u_i - v_i;

                float next_w_r = w_r * wlen_r - w_i * wlen_i;
                float next_w_i = w_r * wlen_i + w_i * wlen_r;
                w_r = next_w_r;
                w_i = next_w_i;
            }
        }
    }
}

void ComputeMagnitudes(void)
{
    for (uint16_t i = 0; i < FFT_N / 2; i++)
    {
        Magnitudes[i] = sqrtf(Acceleration[i] * Acceleration[i] + Acceleration_img[i] * Acceleration_img[i]);
    }
}

int main(void)
{
    clock_init();
    I2Csetup();
    ExternalInterruptInit();
    MpuInit();

    while (1)
    {
        if (read)
        {
            read--;
            ReadMpu();

            if (sample_count < FFT_N)
            {
                float x = (float)accel_x;
                float y = (float)accel_y;
                float z = (float)accel_z;

                Acceleration[sample_count] = sqrtf(x * x + y * y + z * z);
                Acceleration_img[sample_count] = 0.0f;
                sample_count++;

                if (sample_count >= FFT_N)
                {
                    fft_done = 1;
                }
            }
        }

        if (fft_done)
        {
            RemoveOffset();
            FFT(Acceleration, Acceleration_img, FFT_N);
            ComputeMagnitudes();

            sample_count = 0;
            fft_done = 0;
        }
    }
}