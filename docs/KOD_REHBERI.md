# Kodu okuma rehberi

## Başlangıç akışı

`Core/Src/main.c`: `HAL_Init()` → saat → GPIO → 1500 ms bekleme ve I2C hattı kurtarma → I2C1/SPI1/TIM2/USB başlangıcı → TIM2 sayacını başlatma → `osKernelInitialize()` → beş görev oluşturma → `osKernelStart()`.

Görev girişleri `Core/Src/freertos.c` içindedir. `main.c` görevleri oluşturur; `freertos.c` içinde ayrı bir `MX_FREERTOS_Init()` çağrısı beklemeyin.

| Görev | İşlev | Döngü beklemesi | Stack / öncelik |
| --- | --- | --- | --- |
| ControlTask | Manuel/otonom hareket, fren ve reset girişleri | 10 ms | 2048 bayt / Realtime |
| NavigationTask | Omron filtreleme, enkoder yedeği, konum/hız | 1 ms | 4096 bayt / Low |
| IMUTask | Şimdilik yalnızca bekler; IMU okuması yok | 1000 ms | 2048 bayt / Low |
| SensorTask | DS18B20 keşfi ve üç sabit ROM'dan okuma | Her dönüşümde 750 ms, döngü sonunda 2000 ms | 2048 bayt / Low |
| TelemetryTask | USB CDC metin çıktısı | Döngü 10 ms, gönderim 500 ms | 4096 bayt / Low |

Bekleme süreleri kesin görev periyodu değildir; işlem ve zamanlayıcı gecikmeleri eklenir.

## Navigasyon ve kontrol

`OmronlariTara()` üç reflektör girişini tarar. `MesafeHesapla()` pistteki 73 reflektörün aralıklarını kod içinde tanımlar. `HAL_GPIO_EXTI_Callback()` enkoder kenarlarını süre/duty filtresinden geçirir. `yedek_mod=0` Omron, `yedek_mod=1` enkoder kaynağını seçer. Konum çıkışı 0–186 m ile sınırlandırılır.

`OlcumuBaslat()` ölçümleri sıfırlar. Manuel ileri girişinin yükselen kenarı veya otonom istek ölçümü başlatabilir. Otonom akış `OTO_ILERI → OTO_ILERI_FREN → OTO_GERI → OTO_GERI_FREN → OTO_TAMAMLANDI` durumlarından oluşur. Eşikler kaynakta ileri için 50 m, geri için 10 m'dir. Manuel müdahale otonom akışı keser; ayrıntılı koşullar `StartControlTask()` içindedir.

## USB telemetri

`StartTelemetryTask()` satır sonu `\n` olan, virgülle ayrılmış `anahtar:değer` metni gönderir. Bu JSON değildir.

```text
LX:0.000,AX:0.000,SX:0.000,AS:0.000,SE:0.000,MT1:25.00,MT2:25.00,MT3:25.00,RC1:0,RC2:0,RC3:0,F:0,B:0,BR:0,E:1
```

Yukarıdaki değerler yalnızca biçim örneğidir.

| Alan | Anlam |
| --- | --- |
| LX | Seçili kaynaktan konum, m |
| AX | İvme, m/s² |
| SX / AS | Hız / ortalama hız, m/s |
| SE | Enkoder kaynaklı `hallKonum`, m |
| MT1/2/3 | Sıcaklık, °C; başarısız okumada -100 |
| RC1/2/3 | Reflektör sayaçları |
| F / B / BR / E | İleri / geri / fren / boşta durum bayrakları |

USB gönderimi meşgulse bu kod aynı paketi kuyruklayıp tekrar denemez. Telemetri değişkenleri görevler arasında paylaşılır; tek bir zaman noktasının atomik ölçüm paketi garantisi yoktur.

## Diğer dosyalar

- `Core/Inc/main.h`: Sinyal adları ve GPIO eşlemeleri.
- `Core/Inc/FreeRTOSConfig.h`: Heap, tick ve RTOS seçenekleri.
- `Core/Src/onewire.c`: DS18B20 ROM arama, bit zamanlaması ve CRC.
- `Core/Src/madgwick.c`: Filtre kodu mevcut; IMUTask şu anda bunu kullanmıyor.
- `Core/Src/stm32f4xx_it.c`: Kesme yönlendirmeleri.
- `Core/Src/stm32f4xx_hal_msp.c`: Çevrebirim GPIO/saat ayarları.
- `USB_DEVICE/App/usbd_cdc_if.c`: USB CDC arayüzü.

Değişiklikleri ayrı Git dalında yapın. CubeMX kod üretiminden önce commit alın; sonrasında özellikle main.c, freertos.c, GPIO ve USB başlangıç değişikliklerini karşılaştırın.
