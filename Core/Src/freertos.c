/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "math.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "onewire.h"
#include "usbd_cdc_if.h"
#include "usb_device.h"
#include <stdio.h>
#include <stdbool.h>

extern TIM_HandleTypeDef htim2;
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

volatile float LX = 0.0f;
volatile float SX = 0.0f;
volatile float AS = 0.0f;
volatile float AX = 0.0f;

volatile uint32_t RC1 = 0;
volatile uint32_t RC2 = 0;
volatile uint32_t RC3 = 0;

volatile int32_t TekerlekTur = 0;

volatile uint8_t yon_ileri = 1;

/*
 * 0 = Omron ana kaynak
 * 1 = Enkoder yedek kaynak
 */
volatile uint8_t yedek_mod = 0;

volatile uint8_t debug_oto_durum = 0;

volatile float MT1 = 0.0f;
volatile float MT2 = 0.0f;
volatile float MT3 = 0.0f;

volatile uint8_t sicaklik1_ok = 0U;
volatile uint8_t sicaklik2_ok = 0U;
volatile uint8_t sicaklik3_ok = 0U;

volatile uint32_t HareketSuresi_ms = 0;
volatile uint8_t timerBasladi = 0;
volatile uint8_t tekerlek_durdu = 1;
volatile uint8_t oto_durum = 0;

/* Mekanik ve zaman sabitleri */
#define TEKER_CEVRESI_M               0.172788f
#define ENCODER_PPR                    400U
#define ENCODER_MIN_PERIYOT_US         18U
#define ENCODER_MIN_LOW_US             2U
#define ENCODER_MIN_HIGH_US            2U
#define ENCODER_DUTY_MIN_YUZDE         10U
#define ENCODER_DUTY_MAX_YUZDE         90U
#define ENCODER_MIN_TUR_SURESI_US      6500U
#define DURMA_ZAMAN_ASIMI_MS          2000U
#define OMRON_FILTER_SEVIYESI         2U
#define OMRON_ESLESME_BEKLEME_MS      500U
#define OMRON_HIZ_ZAMAN_ASIMI_MS      10000U
#define OTONOM_ILERI_FREN_M           50.0f
#define OTONOM_GERI_FREN_M            10.0f

typedef struct
{
    uint8_t integrator;
    GPIO_PinState kararli_durum;
    uint8_t kilitli;
    uint8_t goruldu;
} OmronFiltre_t;

typedef enum
{
    OTO_BOSTA = 0,
    OTO_ILERI,
    OTO_ILERI_FREN,
    OTO_GERI,
    OTO_GERI_FREN,
    OTO_TAMAMLANDI
} OtonomDurum_t;

static OmronFiltre_t omronFiltre[3];

static volatile uint32_t omronOlayNo = 0;

static volatile uint8_t omronSaglikli = 1;
static volatile uint8_t omronBeklemede = 0;
static volatile uint8_t omronDogrulandi = 0;
static volatile uint32_t omronReddedilen = 0;

static uint32_t omronBeklemeBaslangic_ms = 0;
static uint32_t omronSonDogrulama_ms = 0;

static uint8_t omronReferansVar = 0;
static uint32_t omronSonReflektorNo = 0;

static float omronKonum = 0.0f;
static float omronHiz = 0.0f;
static float omronOrtalamaHiz = 0.0f;
static float omronIvme = 0.0f;
static float omronOncekiHiz = 0.0f;
static float omronToplamYol = 0.0f;

static volatile uint8_t omronGecisDenendiIleri = 0U;
static volatile uint8_t omronGecisDenendiGeri = 0U;
static volatile uint8_t omronGecisHedefAktif = 0U;
static volatile uint32_t omronGecisHedefReflektor = 0U;
static volatile int8_t omronGecisHedefYonu = 0;
static float omronGecisHedefKonum = 0.0f;

static volatile uint32_t hallSonPals_us = 0;
static volatile uint32_t hallPeriyot_us = 0;
static volatile uint32_t hallSonPals_ms = 0;
static volatile uint32_t hallToplamPals = 0;

static volatile uint8_t hallKullanilabilir = 0;
static volatile int8_t hareketYonu = 1;

static volatile uint16_t encoderPalsSayaci = 0U;
static volatile uint32_t encoderSonTamTur_us = 0U;
static volatile uint32_t encoderTurBaslangic_us = 0U;
static volatile uint32_t encoderSonYukselen_us = 0U;
static volatile uint32_t encoderDusenKenar_us = 0U;
static volatile uint8_t encoderLowBasladi = 0U;
static volatile uint8_t encoderIlkPalsAday = 0U;
static volatile uint8_t encoderFiltreKilitli = 0U;
static volatile uint32_t encoderKabulEdilenPals = 0U;
static volatile uint32_t encoderReddedilenPals = 0U;
static volatile uint32_t encoderSonGecerliPals_ms = 0U;

static float hallKonum = 0.0f;
static float hallHiz = 0.0f;
static float hallOrtalamaHiz = 0.0f;
static float hallIvme = 0.0f;
static float hallOncekiHiz = 0.0f;

static uint32_t hareketBaslangic_ms = 0;

static const uint8_t sensor1_id[8] =
{
    0x28, 0x6A, 0xEB, 0x77,
    0x00, 0x00, 0x00, 0x96
};

static const uint8_t sensor2_id[8] =
{
    0x28, 0x25, 0x08, 0x79,
    0x00, 0x00, 0x00, 0x46
};

static const uint8_t sensor3_id[8] =
{
    0x28, 0xF5, 0x1B, 0x78,
    0x00, 0x00, 0x00, 0x3F
};

/* USER CODE END Variables */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

void StartControlTask(void *argument);
void StartNavigationTask(void *argument);
void StartIMUTask(void *argument);
void StartSensorTask(void *argument);
void StartTelemetryTask(void *argument);

void TerminalKomutAl(uint8_t *veri, uint32_t uzunluk);

/* USER CODE END FunctionPrototypes */

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

uint32_t microsSTM(void)
{
    return __HAL_TIM_GET_COUNTER(&htim2);
}

/* ========================================================= */
/*                  USB KOMUT GIRISI KAPALI                  */
/* ========================================================= */

void TerminalKomutAl(uint8_t *veri, uint32_t uzunluk)
{
    (void)veri;
    (void)uzunluk;
}

/* ========================================================= */
/*                     MOTOR KONTROL                         */
/* ========================================================= */

static void MotorlariDurdur(void)
{
    HAL_GPIO_WritePin(
        motor_ileri_GPIO_Port,
        motor_ileri_Pin,
        GPIO_PIN_SET);

    HAL_GPIO_WritePin(
        motor_geri_GPIO_Port,
        motor_geri_Pin,
        GPIO_PIN_SET);

    HAL_GPIO_WritePin(
        s1_GPIO_Port,
        s1_Pin,
        GPIO_PIN_SET);
}

static void FrenleriAyarla(uint8_t fren1, uint8_t fren2)
{
    HAL_GPIO_WritePin(
        fren_cikis1_GPIO_Port,
        fren_cikis1_Pin,
        fren1 ? GPIO_PIN_RESET : GPIO_PIN_SET);

    HAL_GPIO_WritePin(
        fren_cikis2_GPIO_Port,
        fren_cikis2_Pin,
        fren2 ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

static void MotorIleri(void)
{
    hareketYonu = 1;
    yon_ileri = 1;

    FrenleriAyarla(0, 0);

    HAL_GPIO_WritePin(
        motor_geri_GPIO_Port,
        motor_geri_Pin,
        GPIO_PIN_SET);

    HAL_GPIO_WritePin(
        s1_GPIO_Port,
        s1_Pin,
        GPIO_PIN_SET);

    HAL_GPIO_WritePin(
        motor_ileri_GPIO_Port,
        motor_ileri_Pin,
        GPIO_PIN_RESET);
}

static void MotorGeri(void)
{
    hareketYonu = -1;
    yon_ileri = 0;

    FrenleriAyarla(0, 0);

    HAL_GPIO_WritePin(
        motor_ileri_GPIO_Port,
        motor_ileri_Pin,
        GPIO_PIN_SET);

    HAL_GPIO_WritePin(
        s1_GPIO_Port,
        s1_Pin,
        GPIO_PIN_RESET);

    HAL_GPIO_WritePin(
        motor_geri_GPIO_Port,
        motor_geri_Pin,
        GPIO_PIN_RESET);
}

/* ========================================================= */
/*                    ÖLÇÜMÜ BAŞLAT                          */
/* ========================================================= */

static void OlcumuBaslat(void)
{
    uint32_t simdi = HAL_GetTick();

    taskENTER_CRITICAL();

    LX = 0.0f;
    SX = 0.0f;
    AS = 0.0f;
    AX = 0.0f;

    RC1 = 0;
    RC2 = 0;
    RC3 = 0;

    TekerlekTur = 0;

    hallSonPals_us = 0U;
    hallPeriyot_us = 0U;
    hallSonPals_ms = simdi;
    hallToplamPals = 0U;
    hallKullanilabilir = 0U;

    encoderPalsSayaci = 0U;
    encoderSonTamTur_us = 0U;
    encoderTurBaslangic_us = 0U;
    encoderSonYukselen_us = 0U;
    encoderDusenKenar_us = 0U;
    encoderLowBasladi = 0U;
    encoderIlkPalsAday = 0U;
    encoderFiltreKilitli = 0U;
    encoderKabulEdilenPals = 0U;
    encoderReddedilenPals = 0U;
    encoderSonGecerliPals_ms = 0U;

    omronOlayNo = 0U;

    taskEXIT_CRITICAL();

    for (uint8_t i = 0; i < 3; i++)
    {
        omronFiltre[i].integrator = 0;
        omronFiltre[i].kararli_durum = GPIO_PIN_SET;
        omronFiltre[i].kilitli = 0;
        omronFiltre[i].goruldu = 0;
    }

    omronSaglikli = 1;
    omronBeklemede = 0;
    omronDogrulandi = 0;
    omronReddedilen = 0;

    omronBeklemeBaslangic_ms = 0;
    omronSonDogrulama_ms = 0;

    omronReferansVar = 0;
    omronSonReflektorNo = 0;

    omronKonum = 0.0f;
    omronHiz = 0.0f;
    omronOrtalamaHiz = 0.0f;
    omronIvme = 0.0f;
    omronOncekiHiz = 0.0f;
    omronToplamYol = 0.0f;

    omronGecisDenendiIleri = 0U;
    omronGecisDenendiGeri = 0U;
    omronGecisHedefAktif = 0U;
    omronGecisHedefReflektor = 0U;
    omronGecisHedefYonu = 0;
    omronGecisHedefKonum = 0.0f;

    hallKonum = 0.0f;
    hallHiz = 0.0f;
    hallOrtalamaHiz = 0.0f;
    hallIvme = 0.0f;
    hallOncekiHiz = 0.0f;

    yedek_mod = 0;

    hareketBaslangic_ms = simdi;
    HareketSuresi_ms = 0;

    tekerlek_durdu = 1;
    timerBasladi = 1;
}

static void SistemiSifirla(void)
{
    MotorlariDurdur();
    FrenleriAyarla(1, 1);

    timerBasladi = 0;
    oto_durum = OTO_BOSTA;

    OlcumuBaslat();

    timerBasladi = 0;
}

/* ========================================================= */
/*                     CONTROL TASK                          */
/* ========================================================= */

void StartControlTask(void *argument)
{
    uint8_t onceki_otonom = 0;
    uint8_t onceki_ileri = 0;
    uint8_t otonom_aktif = 0;
    uint8_t otonom_manuel_kilit = 0;

    HAL_GPIO_WritePin(
        canlilik_sinyali_GPIO_Port,
        canlilik_sinyali_Pin,
        GPIO_PIN_SET);

    MotorlariDurdur();
    FrenleriAyarla(0, 0);

    for (;;)
    {
        uint8_t ileri =
            (HAL_GPIO_ReadPin(
                Ileri_GPIO_Port,
                Ileri_Pin) == GPIO_PIN_SET);

        uint8_t geri =
            (HAL_GPIO_ReadPin(
                Geri_GPIO_Port,
                Geri_Pin) == GPIO_PIN_SET);

        uint8_t fren1 =
            (HAL_GPIO_ReadPin(
                fren_giris1_GPIO_Port,
                fren_giris1_Pin) == GPIO_PIN_SET);

        uint8_t fren2 =
            (HAL_GPIO_ReadPin(
                fren_giris2_GPIO_Port,
                fren_giris2_Pin) == GPIO_PIN_SET);

        uint8_t otonom_istek =
            (HAL_GPIO_ReadPin(
                otonom_GPIO_Port,
                otonom_Pin) == GPIO_PIN_SET);

        uint8_t manuel_mudahale =
            (ileri || geri || fren1 || fren2) ? 1U : 0U;

        if (!otonom_istek)
        {
            otonom_aktif = 0U;
            otonom_manuel_kilit = 0U;
        }

        if (!onceki_otonom && otonom_istek)
        {
            OlcumuBaslat();
            oto_durum = OTO_ILERI;
            otonom_aktif = 1U;
            otonom_manuel_kilit = 0U;
        }

        if (otonom_aktif && manuel_mudahale)
        {
            otonom_aktif = 0U;
            otonom_manuel_kilit = 1U;
            oto_durum = OTO_BOSTA;
        }

        if (!timerBasladi &&
            !otonom_aktif &&
            !onceki_ileri &&
            ileri)
        {
            OlcumuBaslat();
        }

        if (HAL_GPIO_ReadPin(
                Yazilim_Reset_GPIO_Port,
                Yazilim_Reset_Pin) == GPIO_PIN_SET)
        {
            SistemiSifirla();
        }
        else if (fren1 || fren2)
        {
            MotorlariDurdur();
            FrenleriAyarla(fren1, fren2);
            oto_durum = OTO_BOSTA;
        }
        else if (otonom_aktif &&
                 otonom_istek &&
                 !otonom_manuel_kilit)
        {
            switch ((OtonomDurum_t)oto_durum)
            {
                case OTO_ILERI:
                    MotorIleri();

                    if (LX >= OTONOM_ILERI_FREN_M)
                    {
                        MotorlariDurdur();
                        FrenleriAyarla(1, 1);
                        oto_durum = OTO_ILERI_FREN;
                    }
                    break;

                case OTO_ILERI_FREN:
                    MotorlariDurdur();
                    FrenleriAyarla(1, 1);

                    if (tekerlek_durdu &&
                        hallToplamPals > 0U)
                    {
                        FrenleriAyarla(0, 0);
                        oto_durum = OTO_GERI;
                    }
                    break;

                case OTO_GERI:
                    if (tekerlek_durdu ||
                        hareketYonu < 0)
                    {
                        MotorGeri();
                    }

                    if (LX <= OTONOM_GERI_FREN_M)
                    {
                        MotorlariDurdur();
                        FrenleriAyarla(1, 1);
                        oto_durum = OTO_GERI_FREN;
                    }
                    break;

                case OTO_GERI_FREN:
                    MotorlariDurdur();
                    FrenleriAyarla(1, 1);

                    if (tekerlek_durdu &&
                        hallToplamPals > 0U)
                    {
                        FrenleriAyarla(0, 0);
                        oto_durum = OTO_TAMAMLANDI;
                    }
                    break;

                case OTO_TAMAMLANDI:
                default:
                    MotorlariDurdur();
                    FrenleriAyarla(0, 0);
                    break;
            }
        }
        else
        {
            oto_durum = OTO_BOSTA;

            if (ileri &&
                !geri &&
                !fren1 &&
                !fren2 &&
                (tekerlek_durdu ||
                 hareketYonu > 0))
            {
                MotorIleri();
            }
            else if (geri &&
                     !ileri &&
                     !fren1 &&
                     !fren2 &&
                     (tekerlek_durdu ||
                      hareketYonu < 0))
            {
                MotorGeri();
            }
            else
            {
                MotorlariDurdur();
                FrenleriAyarla(fren1, fren2);
            }
        }

        onceki_otonom = otonom_istek;
        onceki_ileri = ileri;

        debug_oto_durum = oto_durum;

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/* ========================================================= */
/*                REFLEKTÖR MESAFE HARİTASI                  */
/* ========================================================= */

static float MesafeHesapla(uint32_t R)
{
    if (R > 73U)
    {
        R = 73U;
    }

    float metre = 0.0f;

    for (uint32_t i = 1; i <= R; i++)
    {
        if (i == 1U)
        {
            metre += 6.0f;
        }
        else if (i <= 20U)
        {
            metre += 4.0f;
        }
        else if (i <= 39U)
        {
            metre += 0.1f;
        }
        else if (i == 40U)
        {
            metre += 2.1f;
        }
        else if (i <= 52U)
        {
            metre += 4.0f;
        }
        else if (i <= 61U)
        {
            metre += 0.1f;
        }
        else if (i == 62U)
        {
            metre += 3.1f;
        }
        else if (i <= 73U)
        {
            metre += 4.0f;
        }
    }

    return metre;
}

/* ========================================================= */
/*                    OMRON OKUMA                            */
/* ========================================================= */

static void OmronSayaciniGuncelle(uint8_t indeks)
{
    volatile uint32_t *sayaclar[3] =
    {
        &RC1,
        &RC2,
        &RC3
    };

    if (hareketYonu > 0)
    {
        if (*sayaclar[indeks] < 73U)
        {
            (*sayaclar[indeks])++;
        }
    }
    else
    {
        if (*sayaclar[indeks] > 0U)
        {
            (*sayaclar[indeks])--;
        }
    }

    omronFiltre[indeks].goruldu = 1;
    omronOlayNo++;
}

static void OmronFiltrele(
    uint8_t indeks,
    GPIO_PinState ham)
{
    OmronFiltre_t *filtre =
        &omronFiltre[indeks];

    if (ham == GPIO_PIN_RESET)
    {
        if (filtre->integrator <
            OMRON_FILTER_SEVIYESI)
        {
            filtre->integrator++;
        }

        if (filtre->integrator >=
                OMRON_FILTER_SEVIYESI &&
            filtre->kararli_durum !=
                GPIO_PIN_RESET)
        {
            filtre->kararli_durum =
                GPIO_PIN_RESET;

            if (!filtre->kilitli)
            {
                filtre->kilitli = 1;
                OmronSayaciniGuncelle(indeks);
            }
        }
    }
    else
    {
        if (filtre->integrator > 0U)
        {
            filtre->integrator--;
        }

        if (filtre->integrator == 0U)
        {
            filtre->kararli_durum =
                GPIO_PIN_SET;

            filtre->kilitli = 0;
        }
    }
}

static void OmronlariTara(void)
{
    if (!timerBasladi)
    {
        return;
    }

    OmronFiltrele(
        0,
        HAL_GPIO_ReadPin(
            Sensor1_GPIO_Port,
            Sensor1_Pin));

    OmronFiltrele(
        1,
        HAL_GPIO_ReadPin(
            Sensor2_GPIO_Port,
            Sensor2_Pin));

    OmronFiltrele(
        2,
        HAL_GPIO_ReadPin(
            Sensor3_GPIO_Port,
            Sensor3_Pin));
}

/* ========================================================= */
/*                  OMRON ÇOĞUNLUK KARARI                    */
/* ========================================================= */

static bool OmronCiftDogrulama(
    uint32_t *reflektorNo)
{
    uint32_t r1 = RC1;
    uint32_t r2 = RC2;
    uint32_t r3 = RC3;

    if (omronFiltre[0].goruldu &&
        omronFiltre[1].goruldu &&
        r1 == r2)
    {
        *reflektorNo = r1;
        return true;
    }

    if (omronFiltre[0].goruldu &&
        omronFiltre[2].goruldu &&
        r1 == r3)
    {
        *reflektorNo = r1;
        return true;
    }

    if (omronFiltre[1].goruldu &&
        omronFiltre[2].goruldu &&
        r2 == r3)
    {
        *reflektorNo = r2;
        return true;
    }

    return false;
}

static bool OmronAdayiYeniMi(uint32_t aday)
{
    if (!omronReferansVar)
    {
        return true;
    }

    if (hareketYonu > 0)
    {
        return (aday > omronSonReflektorNo);
    }

    return (aday < omronSonReflektorNo);
}

static bool OmronlarSonReferanstaTamamlandi(void)
{
    if (!omronReferansVar)
    {
        return false;
    }

    uint8_t gorulenSensor = 0;
    uint8_t referanstaSensor = 0;

    if (omronFiltre[0].goruldu)
    {
        gorulenSensor++;

        if (RC1 == omronSonReflektorNo)
        {
            referanstaSensor++;
        }
    }

    if (omronFiltre[1].goruldu)
    {
        gorulenSensor++;

        if (RC2 == omronSonReflektorNo)
        {
            referanstaSensor++;
        }
    }

    if (omronFiltre[2].goruldu)
    {
        gorulenSensor++;

        if (RC3 == omronSonReflektorNo)
        {
            referanstaSensor++;
        }
    }

    return (gorulenSensor >= 2U &&
            gorulenSensor == referanstaSensor);
}

/* ========================================================= */
/*              OMRON REFERANSINI KABUL ET                   */
/* ========================================================= */

static void OmronReferansiniKabulEt(
    uint32_t reflektorNo,
    uint32_t simdi_ms)
{
    float yeniKonum =
        MesafeHesapla(reflektorNo);

    if (omronReferansVar)
    {
        uint32_t dt_ms =
            simdi_ms -
            omronSonDogrulama_ms;

        float dx =
            yeniKonum -
            omronKonum;

        if (dt_ms > 0U &&
            fabsf(dx) > 0.001f)
        {
            float dt =
                (float)dt_ms /
                1000.0f;

            float yeniHiz =
                dx / dt;

            omronToplamYol +=
                fabsf(dx);

            omronHiz =
                (0.60f * omronHiz) +
                (0.40f * yeniHiz);

            omronIvme =
                (0.70f * omronIvme) +
                (0.30f *
                 ((omronHiz -
                   omronOncekiHiz) / dt));

            omronOncekiHiz =
                omronHiz;
        }
    }
    else
    {
        omronToplamYol =
            fabsf(yeniKonum);
    }

    omronKonum =
        yeniKonum;

    omronSonReflektorNo =
        reflektorNo;

    omronSonDogrulama_ms =
        simdi_ms;

    omronReferansVar = 1;
    omronSaglikli = 1;
    omronBeklemede = 0;
    omronDogrulandi = 1;

    yedek_mod = 0;
}

/* ========================================================= */
/*       ENKODER YEDEĞİNDEN OMRON'A KONTROLLÜ DÖNÜŞ         */
/* ========================================================= */

static void OmronGecisHedefiniKur(void)
{
    uint32_t hedef = 0U;

    if (hareketYonu > 0)
    {
        if (omronGecisDenendiIleri)
        {
            return;
        }

        for (uint32_t i = 1U; i <= 73U; i++)
        {
            if (MesafeHesapla(i) >
                (hallKonum + 0.001f))
            {
                hedef = i;
                break;
            }
        }
    }
    else
    {
        if (omronGecisDenendiGeri)
        {
            return;
        }

        for (int32_t i = 73; i >= 1; i--)
        {
            if (MesafeHesapla((uint32_t)i) <
                (hallKonum - 0.001f))
            {
                hedef = (uint32_t)i;
                break;
            }
        }
    }

    if (hedef == 0U)
    {
        return;
    }

    omronGecisHedefReflektor = hedef;
    omronGecisHedefKonum = MesafeHesapla(hedef);
    omronGecisHedefYonu = hareketYonu;
    omronGecisHedefAktif = 1U;
}

static void OmronYenidenGecisiDene(uint32_t simdi_ms)
{
    uint8_t hedefeGelindi;

    if (omronSaglikli)
    {
        return;
    }

    if (!omronGecisHedefAktif)
    {
        OmronGecisHedefiniKur();

        if (!omronGecisHedefAktif)
        {
            return;
        }
    }

    if (hareketYonu != omronGecisHedefYonu)
    {
        omronGecisHedefAktif = 0U;
        OmronGecisHedefiniKur();
        return;
    }

    if (hareketYonu > 0)
    {
        hedefeGelindi =
            (hallKonum >= omronGecisHedefKonum);
    }
    else
    {
        hedefeGelindi =
            (hallKonum <= omronGecisHedefKonum);
    }

    if (!hedefeGelindi)
    {
        return;
    }

    if (hareketYonu > 0)
    {
        omronGecisDenendiIleri = 1U;
    }
    else
    {
        omronGecisDenendiGeri = 1U;
    }

    omronGecisHedefAktif = 0U;

    taskENTER_CRITICAL();

    RC1 = omronGecisHedefReflektor;
    RC2 = omronGecisHedefReflektor;
    RC3 = omronGecisHedefReflektor;

    taskEXIT_CRITICAL();

    OmronReferansiniKabulEt(
        omronGecisHedefReflektor,
        simdi_ms);
}

/* ========================================================= */
/*                 OMRON SAĞLIK KONTROLÜ                     */
/* ========================================================= */

static void OmronDurumunuDegerlendir(
    uint32_t simdi_ms,
    uint32_t *oncekiOmronOlay)
{
    uint32_t mevcutOlay =
        omronOlayNo;

    if (!omronSaglikli)
    {
        *oncekiOmronOlay = mevcutOlay;
        omronBeklemede = 0U;
        return;
    }

    if (mevcutOlay != *oncekiOmronOlay)
    {
        *oncekiOmronOlay =
            mevcutOlay;

        if (!omronBeklemede)
        {
            omronBeklemede = 1;
            omronBeklemeBaslangic_ms =
                simdi_ms;
        }

        uint32_t adayReflektor = 0;

        if (OmronCiftDogrulama(
                &adayReflektor))
        {
            if (OmronAdayiYeniMi(
                    adayReflektor))
            {
                OmronReferansiniKabulEt(
                    adayReflektor,
                    simdi_ms);

                return;
            }

            if (OmronlarSonReferanstaTamamlandi())
            {
                omronSaglikli = 1;
                omronDogrulandi = 1;
                omronBeklemede = 0;
                yedek_mod = 0;

                return;
            }
        }
    }

    if (omronBeklemede &&
        (simdi_ms -
         omronBeklemeBaslangic_ms) >=
            OMRON_ESLESME_BEKLEME_MS)
    {
        uint32_t adayReflektor = 0;

        if (OmronCiftDogrulama(
                &adayReflektor))
        {
            if (OmronAdayiYeniMi(
                    adayReflektor))
            {
                OmronReferansiniKabulEt(
                    adayReflektor,
                    simdi_ms);

                return;
            }

            if (OmronlarSonReferanstaTamamlandi())
            {
                omronSaglikli = 1;
                omronDogrulandi = 1;
                omronBeklemede = 0;
                yedek_mod = 0;

                return;
            }
        }

        omronSaglikli = 0;
        omronDogrulandi = 0;
        omronBeklemede = 0;
        omronReddedilen++;

        yedek_mod = 1;
        OmronGecisHedefiniKur();
    }
}

/* ========================================================= */
/*                    NAVIGATION TASK                        */
/* ========================================================= */

void StartNavigationTask(void *argument)
{
    uint32_t oncekiKinematik_us =
        microsSTM();

    uint32_t oncekiOmronOlay = 0;

    for (;;)
    {
        uint32_t simdi_us =
            microsSTM();

        uint32_t simdi_ms =
            HAL_GetTick();

        OmronlariTara();

        if (!timerBasladi)
        {
            LX = 0.0f;
            SX = 0.0f;
            AS = 0.0f;
            AX = 0.0f;

            vTaskDelay(pdMS_TO_TICKS(1));
            continue;
        }

        HareketSuresi_ms =
            simdi_ms -
            hareketBaslangic_ms;

        if (encoderKabulEdilenPals == 0U ||
            encoderSonGecerliPals_ms == 0U)
        {
            tekerlek_durdu = 1;
        }
        else if ((simdi_ms -
                  encoderSonGecerliPals_ms) >=
                    DURMA_ZAMAN_ASIMI_MS)
        {
            tekerlek_durdu = 1;
        }
        else
        {
            tekerlek_durdu = 0;
        }

        hallKonum =
            (float)TekerlekTur *
            TEKER_CEVRESI_M;

        if (hallKonum < 0.0f)
        {
            hallKonum = 0.0f;
        }

        if (hallKonum > 186.0f)
        {
            hallKonum = 186.0f;
        }

        if ((uint32_t)(
                simdi_us -
                oncekiKinematik_us) >=
            20000U)
        {
            float dt =
                (float)(
                    simdi_us -
                    oncekiKinematik_us) /
                1000000.0f;

            float hallOlculenHiz =
                0.0f;

            if (!tekerlek_durdu &&
                hallPeriyot_us > 0U)
            {
                hallOlculenHiz =
                    ((float)hareketYonu *
                     TEKER_CEVRESI_M *
                     1000000.0f) /
                    (float)hallPeriyot_us;
            }

            hallHiz =
                (0.65f * hallHiz) +
                (0.35f *
                 hallOlculenHiz);

            if (tekerlek_durdu &&
                fabsf(hallHiz) < 0.02f)
            {
                hallHiz = 0.0f;
            }

            hallIvme =
                (0.70f * hallIvme) +
                (0.30f *
                 ((hallHiz -
                   hallOncekiHiz) / dt));

            if (tekerlek_durdu &&
                hallHiz == 0.0f &&
                fabsf(hallIvme) < 0.05f)
            {
                hallIvme = 0.0f;
            }

            hallOncekiHiz =
                hallHiz;

            oncekiKinematik_us =
                simdi_us;
        }

        if (HareketSuresi_ms > 0U)
        {
            float sure_s =
                (float)HareketSuresi_ms /
                1000.0f;

            hallOrtalamaHiz =
                ((float)hallToplamPals *
                 TEKER_CEVRESI_M) /
                sure_s;

            omronOrtalamaHiz =
                omronToplamYol /
                sure_s;
        }

        OmronDurumunuDegerlendir(
            simdi_ms,
            &oncekiOmronOlay);

        OmronYenidenGecisiDene(simdi_ms);

        if (omronReferansVar &&
            omronSonDogrulama_ms != 0U &&
            (simdi_ms -
             omronSonDogrulama_ms) >=
                OMRON_HIZ_ZAMAN_ASIMI_MS)
        {
            omronHiz = 0.0f;
            omronIvme = 0.0f;
            omronOncekiHiz = 0.0f;
        }

        if (!yedek_mod)
        {
            LX = omronKonum;
            SX = omronHiz;
            AS = omronOrtalamaHiz;
            AX = omronIvme;
        }
        else
        {
            LX = hallKonum;
            SX = hallHiz;
            AS = hallOrtalamaHiz;
            AX = hallIvme;
        }

        if (LX < 0.0f)
        {
            LX = 0.0f;
        }

        if (LX > 186.0f)
        {
            LX = 186.0f;
        }

        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

void StartIMUTask(void *argument)
{
    (void)argument;

    for (;;)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* ========================================================= */
/*                    DS18B20 OKUMA                          */
/* ========================================================= */

float DS18B20_ReadTemp(
    const uint8_t *rom_id)
{
    uint8_t data[9];
    float temp = -100.0f;

    if (OneWire_Reset() != 0)
    {
        return temp;
    }

    OneWire_WriteByte(0x55);

    for (int i = 0; i < 8; i++)
    {
        OneWire_WriteByte(
            rom_id[i]);
    }

    OneWire_WriteByte(0x44);

    vTaskDelay(pdMS_TO_TICKS(750));

    if (OneWire_Reset() != 0)
    {
        return temp;
    }

    OneWire_WriteByte(0x55);

    for (int i = 0; i < 8; i++)
    {
        OneWire_WriteByte(
            rom_id[i]);
    }

    OneWire_WriteByte(0xBE);

    for (int i = 0; i < 9; i++)
    {
        data[i] =
            OneWire_ReadByte();
    }

    if (OneWire_CRC8(data, 8) ==
        data[8])
    {
        int16_t raw =
            (int16_t)(
                (data[1] << 8) |
                 data[0]);

        temp =
            (float)raw / 16.0f;
    }

    return temp;
}

volatile uint8_t Bulunan_IDler[3][8] =
{
    {0}
};

volatile uint8_t Bulunan_Sensor_Sayisi = 0;
volatile uint8_t sensor_cevap = 0;

/* ========================================================= */
/*                    SICAKLIK TASK                          */
/* ========================================================= */

void StartSensorTask(void *argument)
{
    OneWire_Init();

    sensor_cevap =
        OneWire_Reset();

    if (OneWire_First())
    {
        do
        {
            if (Bulunan_Sensor_Sayisi < 3)
            {
                OneWire_GetFullROM(
                    (uint8_t *)
                    Bulunan_IDler[
                        Bulunan_Sensor_Sayisi]);

                Bulunan_Sensor_Sayisi++;
            }
        }
        while (OneWire_Next());
    }

    for (;;)
    {
        MT1 =
            DS18B20_ReadTemp(
                sensor1_id);

        sicaklik1_ok =
            (MT1 >= -55.0f && MT1 <= 125.0f);

        MT2 =
            DS18B20_ReadTemp(
                sensor2_id);

        sicaklik2_ok =
            (MT2 >= -55.0f && MT2 <= 125.0f);

        MT3 =
            DS18B20_ReadTemp(
                sensor3_id);

        sicaklik3_ok =
            (MT3 >= -55.0f && MT3 <= 125.0f);

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

/* ========================================================= */
/*                     TELEMETRY TASK                        */
/* ========================================================= */

void StartTelemetryTask(void *argument)
{
    static char telemetryBuffer[512];
    uint32_t last_send_time = 0U;
    const uint32_t send_interval = 500U;

    for (;;)
    {
        uint32_t now = HAL_GetTick();

        if ((now - last_send_time) >= send_interval)
        {
            last_send_time = now;

            uint8_t frenAktif =
                (HAL_GPIO_ReadPin(
                     fren_cikis1_GPIO_Port,
                     fren_cikis1_Pin) == GPIO_PIN_RESET) ||
                (HAL_GPIO_ReadPin(
                     fren_cikis2_GPIO_Port,
                     fren_cikis2_Pin) == GPIO_PIN_RESET);

            uint8_t ileriAktif =
                (HAL_GPIO_ReadPin(
                     motor_ileri_GPIO_Port,
                     motor_ileri_Pin) == GPIO_PIN_RESET) &&
                (HAL_GPIO_ReadPin(
                     motor_geri_GPIO_Port,
                     motor_geri_Pin) == GPIO_PIN_SET);

            uint8_t geriAktif =
                (HAL_GPIO_ReadPin(
                     motor_geri_GPIO_Port,
                     motor_geri_Pin) == GPIO_PIN_RESET) &&
                (HAL_GPIO_ReadPin(
                     motor_ileri_GPIO_Port,
                     motor_ileri_Pin) == GPIO_PIN_SET);

            uint8_t telemetriIleri = 0U;
            uint8_t telemetriGeri = 0U;
            uint8_t telemetriFren = 0U;
            uint8_t telemetriBosta = 0U;

            if (frenAktif)
            {
                telemetriFren = 1U;
            }
            else if (ileriAktif)
            {
                telemetriIleri = 1U;
            }
            else if (geriAktif)
            {
                telemetriGeri = 1U;
            }
            else
            {
                telemetriBosta = 1U;
            }

            int len = snprintf(
                telemetryBuffer,
                sizeof(telemetryBuffer),
                "LX:%.3f,"
                "AX:%.3f,"
                "SX:%.3f,"
                "AS:%.3f,"
                "SE:%.3f,"
                "MT1:%.2f,MT2:%.2f,MT3:%.2f,"
                "RC1:%lu,RC2:%lu,RC3:%lu,"
                "F:%u,B:%u,BR:%u,E:%u\n",
                LX,
                AX,
                SX,
                AS,
                hallKonum,
                MT1,
                MT2,
                MT3,
                (unsigned long)RC1,
                (unsigned long)RC2,
                (unsigned long)RC3,
                (unsigned int)telemetriIleri,
                (unsigned int)telemetriGeri,
                (unsigned int)telemetriFren,
                (unsigned int)telemetriBosta);

            if (len > 0 && len < (int)sizeof(telemetryBuffer))
            {
                if (CDC_Transmit_FS(
                        (uint8_t *)telemetryBuffer,
                        (uint16_t)len) == USBD_OK)
                {
                    HAL_GPIO_WritePin(
                        LD4_GPIO_Port,
                        LD4_Pin,
                        GPIO_PIN_RESET);
                }
                else
                {
                    HAL_GPIO_WritePin(
                        LD4_GPIO_Port,
                        LD4_Pin,
                        GPIO_PIN_SET);
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/* ========================================================= */
/*                ENKODER EXTI KESMESİ                       */
/* ========================================================= */

void HAL_GPIO_EXTI_Callback(
    uint16_t GPIO_Pin)
{
    uint32_t su_an_ms = HAL_GetTick();
    uint32_t simdi_us;
    uint32_t lowSuresi_us;
    uint32_t oncekiYukselen_us;
    uint32_t periyot_us;
    uint32_t highSuresi_us;
    uint32_t dutyYuzde;
    uint32_t turSuresi_us;
    uint16_t eklenecekPals;
    GPIO_PinState pinDurumu;

    if (su_an_ms < 500U ||
        !timerBasladi ||
        GPIO_Pin != hallSensor_Pin)
    {
        return;
    }

    simdi_us = microsSTM();

    pinDurumu = HAL_GPIO_ReadPin(
        hallSensor_GPIO_Port,
        hallSensor_Pin);

    if (pinDurumu == GPIO_PIN_RESET)
    {
        if (encoderLowBasladi)
        {
            encoderReddedilenPals++;
            return;
        }

        encoderDusenKenar_us = simdi_us;
        encoderLowBasladi = 1U;
        return;
    }

    if (!encoderLowBasladi)
    {
        encoderReddedilenPals++;
        return;
    }

    lowSuresi_us =
        simdi_us - encoderDusenKenar_us;

    encoderLowBasladi = 0U;

    if (lowSuresi_us < ENCODER_MIN_LOW_US)
    {
        encoderReddedilenPals++;
        encoderSonYukselen_us = simdi_us;
        encoderIlkPalsAday = 0U;
        return;
    }

    if (encoderSonYukselen_us == 0U)
    {
        encoderSonYukselen_us = simdi_us;
        encoderIlkPalsAday = 1U;
        return;
    }

    oncekiYukselen_us = encoderSonYukselen_us;
    periyot_us = simdi_us - oncekiYukselen_us;
    highSuresi_us = encoderDusenKenar_us - oncekiYukselen_us;

    encoderSonYukselen_us = simdi_us;

    if (periyot_us < ENCODER_MIN_PERIYOT_US ||
        highSuresi_us < ENCODER_MIN_HIGH_US ||
        lowSuresi_us >= periyot_us)
    {
        encoderReddedilenPals++;
        encoderIlkPalsAday = 0U;
        return;
    }

    dutyYuzde =
        (lowSuresi_us * 100U) /
        periyot_us;

    if (dutyYuzde < ENCODER_DUTY_MIN_YUZDE ||
        dutyYuzde > ENCODER_DUTY_MAX_YUZDE)
    {
        encoderReddedilenPals++;
        encoderIlkPalsAday = 0U;
        return;
    }

    eklenecekPals =
        encoderIlkPalsAday ? 2U : 1U;

    encoderIlkPalsAday = 0U;
    encoderFiltreKilitli = 1U;

    if (encoderPalsSayaci == 0U)
    {
        encoderTurBaslangic_us =
            (eklenecekPals == 2U) ?
            oncekiYukselen_us : simdi_us;
    }

    encoderPalsSayaci += eklenecekPals;
    encoderKabulEdilenPals += eklenecekPals;

    encoderSonGecerliPals_ms = su_an_ms;
    hallKullanilabilir = 1U;
    tekerlek_durdu = 0U;

    if (encoderPalsSayaci < ENCODER_PPR)
    {
        return;
    }

    encoderPalsSayaci -= ENCODER_PPR;

    turSuresi_us =
        simdi_us - encoderTurBaslangic_us;

    if (turSuresi_us < ENCODER_MIN_TUR_SURESI_US)
    {
        if (encoderKabulEdilenPals >= ENCODER_PPR)
        {
            encoderKabulEdilenPals -= ENCODER_PPR;
        }

        encoderReddedilenPals += ENCODER_PPR;
        encoderFiltreKilitli = 0U;
        encoderTurBaslangic_us =
            (encoderPalsSayaci > 0U) ? simdi_us : 0U;
        return;
    }

    encoderTurBaslangic_us =
        (encoderPalsSayaci > 0U) ? simdi_us : 0U;

    if (encoderSonTamTur_us != 0U)
    {
        hallPeriyot_us =
            simdi_us - encoderSonTamTur_us;
    }

    encoderSonTamTur_us = simdi_us;
    hallSonPals_us = simdi_us;
    hallSonPals_ms = su_an_ms;
    hallToplamPals++;
    hallKullanilabilir = 1U;
    tekerlek_durdu = 0U;

    if (hareketYonu > 0)
    {
        TekerlekTur++;
    }
    else
    {
        TekerlekTur--;
    }
}

/* ========================================================= */
/*                 STACK OVERFLOW HOOK                       */
/* ========================================================= */

void vApplicationStackOverflowHook(
    TaskHandle_t xTask,
    char *pcTaskName)
{
    (void)xTask;
    (void)pcTaskName;

    while (1)
    {
    }
}

/* USER CODE END Application */
