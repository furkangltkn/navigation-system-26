# Kodu okuma rehberi

İlk kez inceliyorsan önce [README içindeki dosya okuma sırasını](../README.md#önerilen-dosya-okuma-sırası) takip et. Bu belge, oradaki kısa açıklamaların ayrıntılarını verir.

## Kısa sözlük

- **GPIO / pin:** STM32'nin dış devreyle sinyal alışverişi yaptığı bağlantı.
- **Görev / task:** Belirli bir işi yürüten, FreeRTOS tarafından zamanlanan fonksiyon.
- **Kesme / interrupt:** Bir donanım olayı geldiğinde ilgili kodun çalıştırılması; örneğin enkoder sinyalinin değişmesi.
- **Telemetri:** Ölçüm ve durum bilgilerinin bilgisayara gönderilmesi.
- **Heap:** Görevler ve diğer nesneler için çalışma sırasında ayrılan bellek alanı.
- **Stack:** Her görevin fonksiyon çağrıları ve yerel değişkenleri için kullandığı bellek alanı.

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

## Dosya dosya önemli bölümler

Aşağıdaki dosya bağlantıları kodu açar. Tablodaki fonksiyon veya ayar adını dosyada arayarak ilgili bölüme ulaşabilirsin. `.c` dosyası işlemlerin nasıl yapıldığını, `.h` dosyası ise ortak tanımları ve fonksiyon arayüzlerini gösterir.

<a id="main-c"></a>
### main.c — Kartın açılışı ve görevlerin başlatılması

[Kodu aç](../Core/Src/main.c). Önce `main()` fonksiyonunu baştan sona oku; ardından burada çağrılan başlangıç fonksiyonlarına geç.

| Bakılacak bölüm | Ne yapıyor? | Neye dikkat etmelisin? |
| --- | --- | --- |
| `main()` | Donanımı hazırlar ve FreeRTOS'u başlatır. | Başlangıç sırasını takip et. Normal çalışma başladıktan sonra işler alttaki `while (1)` yerine görevlerde yürür. |
| `ControlTask_attributes` ve diğer `*_attributes` | Her görevin stack boyutunu ve önceliğini belirler. | `.stack_size` bayt cinsindedir; örneğin `512 * 4`, 2048 bayttır. Bunları FreeRTOS heap bütçesiyle birlikte değerlendir. |
| `osThreadNew()` çağrıları | Beş görevi oluşturur, tutamaçlarını döndürür. | `ControlTaskHandle` dahil beş tutamacın da NULL olmadığını kontrol et. Mevcut kod sonuçları kontrol etmiyor. |
| `SystemClock_Config()` | İşlemci ve çevrebirimlerin saat hızlarını ayarlar. | Mevcut yapı 8 MHz HSE'den 168 MHz sistem saati üretir. Saat değişimi timer ve haberleşme zamanlamalarını da etkiler. |
| `I2C_Hatti_Kurtar()` | PB6/PB9 hattına saat darbeleri ve STOP sırası uygular. | `main()` içinde I2C başlatılmadan önce çağrılır. Çağrının yeri ve öncesindeki bekleme özel başlangıç kodudur. |
| `MX_GPIO_Init()` | Pin yönünü, pull-up/pull-down ayarlarını ve ilk çıkış seviyelerini belirler. | `main.h` tanımlarıyla karşılaştır. Fren 1 için port tutarsızlığı [doğrulama notlarında](DOGRULAMA.md) açıklanıyor. |
| `MX_I2C1_Init()` / `MX_SPI1_Init()` | I2C ve SPI haberleşme ayarlarını kurar. | Bir arayüzün kurulması, sensörün okunduğu anlamına gelmez; IMUTask şu anda boştur. Fiziksel pin ayarları MSP dosyasındadır. |
| `MX_TIM2_Init()` ve `HAL_TIM_Base_Start()` | Süre ölçümünde kullanılan TIM2 sayacını kurar ve başlatır. | Prescaler 83'tür; mevcut saat düzeninde sayaç 1 µs adımlarla ilerler. `freertos.c` içindeki `microsSTM()` bu sayacı okur. |
| `HAL_TIM_PeriodElapsedCallback()` | TIM6 kesmesinde HAL zaman sayacını ilerletir. | TIM2 ile karıştırma: TIM6, `HAL_GetTick()` / `HAL_Delay()` zaman tabanını besler. |
| `Error_Handler()` | Başlangıç hatasında kesmeleri kapatıp sonsuz döngüye girer. | Kart burada duruyorsa debugger çağrı zincirinden hangi başlangıç işleminin başarısız olduğunu bul. |

**Okuma hedefi:** “Kart açılınca hangi işlemler hangi sırayla yapılıyor ve uygulama görevlerine nerede geçiliyor?” sorusunu cevaplayabilmek.

<a id="main-h"></a>
### main.h — Sinyal isimleri ve pin eşlemesi

[Kodu aç](../Core/Inc/main.h). Bu dosyada çalışan bir kontrol döngüsü yoktur; diğer dosyaların kullandığı isimler tanımlanır.

| Bakılacak tanımlar | Ne anlatıyor? | Neye dikkat etmelisin? |
| --- | --- | --- |
| `hallSensor_Pin`, `hallSensor_GPIO_Port`, `hallSensor_EXTI_IRQn` | Enkoder girişi PE3 ve EXTI3 kesmesi. | Pin, GPIO kurulumu ve kesme yönlendirmesi birlikte eşleşmeli. |
| `Sensor1_Pin` / `Sensor2_Pin` / `Sensor3_Pin` ve portları | Üç Omron girişinin bağlantısı. | Gerçek kablolamayı ve aktif düşük okuma davranışını kontrol et. |
| `Ileri`, `Geri`, `otonom`, `Yazilim_Reset` pin/port çiftleri | Kontrol komutlarının fiziksel girişleri. | Bunlar USB komutları değil, GPIO girişleridir. |
| `motor_ileri`, `motor_geri`, `fren_cikis1`, `fren_cikis2` pin/port çiftleri | Hareket ve fren çıkışlarının bağlantısı. | Pin numarası tek başına yetmez; port da gerekir. `GPIO_PIN_0` farklı portlarda farklı fiziksel pindir. |
| `sicaklik_Pin` ve `sicaklik_GPIO_Port` | PA1 sıcaklık veri hattı. | `onewire.h` içindeki ayrı `ONEWIRE_GPIO_*` tanımlarıyla aynı hattı göstermeli. |

Örnek: `fren_cikis1_Pin = GPIO_PIN_0` ve `fren_cikis1_GPIO_Port = GPIOD` birlikte **PD0** demektir. Bir pinin aktif yüksek/düşük olması bu isimlerden anlaşılmaz; `main.c` kurulumu ve `freertos.c` okuma/yazma işlemleri birlikte incelenir. Tüm bağlantılar için [donanım tablosuna](DONANIM.md) bak.

<a id="onewire"></a>
### onewire.c ve onewire.h — Sıcaklık sensörüyle iletişim

[onewire.c kodunu aç](../Core/Src/onewire.c) · [onewire.h tanımlarını aç](../Core/Inc/onewire.h)

| Bakılacak bölüm | Ne yapıyor? | Neye dikkat etmelisin? |
| --- | --- | --- |
| `ONEWIRE_GPIO_PORT` / `ONEWIRE_GPIO_PIN` | Veri hattını seçer; şu anda PA1. | Pin değişirse `main.h`, GPIO ayarları ve donanım da eşleşmeli. |
| `OneWire_Init()` | Open-drain GPIO'yu ve mikro saniye beklemeleri için DWT sayacını hazırlar. | Okumadan önce çağrılması gerekir; SensorTask bunu başlangıçta yapar. |
| `OneWire_Delay_us()` | İşlemci çevrim sayacından kısa bekleme üretir. | DWT sayacı ilerlemiyorsa bekleme tamamlanmaz. Zamanlamayı işlemci saatinden hesaplar. |
| `OneWire_Reset()` | Hatta reset darbesi gönderir ve sensörün varlık yanıtını okur. | Dönüş değeri **0 ise yanıt var**, sıfır dışı ise yanıt yok şeklinde kullanılıyor. |
| `OneWire_WriteBit()` / `OneWire_ReadBit()` | Tek biti doğru zaman aralıklarıyla gönderir/okur. | Kritik bölümler kısa zamanlamayı korur; süre değişiklikleri haberleşmeyi etkiler. |
| `OneWire_WriteByte()` / `OneWire_ReadByte()` | Bit işlemlerini kullanarak bir bayt aktarır. | Bunlar sıcaklık hesabı yapmaz; ham veri taşır. |
| `OneWire_First()`, `OneWire_Next()`, `OneWire_Search()`, `OneWire_GetFullROM()` | Aynı hattaki sensörlerin ROM kimliklerini keşfeder. | Keşfedilen kimlikler otomatik olarak `sensor1_id/2_id/3_id` yerine geçmez. |
| `OneWire_CRC8()` | Gelen verinin CRC kontrol değerini hesaplar. | CRC karşılaştırması ve sıcaklığa dönüşüm `freertos.c` içindeki `DS18B20_ReadTemp()` fonksiyonundadır. |

**İşlem zinciri:** `StartSensorTask()` → `DS18B20_ReadTemp()` → OneWire işlemleri → CRC kontrolü → `MT1/MT2/MT3`. Sıcaklık -100 olduğunda kablo/kimlik/okuma/CRC zincirini kontrol et; bunu gerçek sıcaklık kabul etme.

<a id="rtos-config"></a>
### FreeRTOSConfig.h — Görevlerin bellek ve zamanlama ayarları

[Kodu aç](../Core/Inc/FreeRTOSConfig.h). Görevlerin yaptığı iş burada yazmaz; görevleri çalıştıran sistemin ayarları burada bulunur.

| Bakılacak ayar | Bu projedeki anlamı | Neye dikkat etmelisin? |
| --- | --- | --- |
| `configTOTAL_HEAP_SIZE` | RTOS dinamik belleği 15360 bayt. | Uygulama stack toplamı 14336 bayt; görev kontrol blokları ve diğer tahsisler de alan ister. [Bellek notunu](DOGRULAMA.md) oku. |
| `configTICK_RATE_HZ` | Saniyede 1000 RTOS tick; nominal adım 1 ms. | `pdMS_TO_TICKS()` görev beklemelerini tick'e çevirir. Bu değer görevlerin tam olarak her 1 ms'de çalışacağını garanti etmez. |
| `configSUPPORT_DYNAMIC_ALLOCATION` / `USE_FreeRTOS_HEAP_4` | Dinamik tahsis ve heap_4 bellek yöneticisi kullanılır. | `osThreadNew()` başarısızlığı ile bu bellek bütçesini birlikte incele. |
| `configUSE_NEWLIB_REENTRANT` | Görevlere ait C kütüphanesi durum bilgisini etkinleştirir. | Görev başına ek bellek ihtiyacını hesaba kat. |
| `configUSE_TIMERS`, `configTIMER_TASK_STACK_DEPTH` | RTOS yazılım timer desteği ve timer görevi stack ayarı. | Bunlar donanım TIM2/TIM6 ayarları değildir. |
| `configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY` | RTOS API kullanan kesmeler için öncelik sınırı; burada 5. | Kesme önceliği ile görev önceliği farklı kavramlardır. Kesme ayarını değiştirirken bu dosyadaki açıklamaları da oku. |
| `configASSERT` | Koşul sağlanmazsa kesmeleri kapatıp bekler. | Burada durmayı normal görev beklemesiyle karıştırma. |

`vApplicationStackOverflowHook()` uygulamada tanımlı olsa da `configCHECK_FOR_STACK_OVERFLOW` bu dosyada etkin değil. Heap toplamı ile her görevin ayrı stack boyutunu farklı ölçümler olarak değerlendir.

<a id="interrupts"></a>
### stm32f4xx_it.c — Donanım olayının uygulamaya ulaşması

[Kodu aç](../Core/Src/stm32f4xx_it.c).

| Bakılacak fonksiyon | Görevi | Takip edilecek bağlantı |
| --- | --- | --- |
| `EXTI3_IRQHandler()` | Enkoder girişinin kesmesini HAL'e iletir. | `HAL_GPIO_EXTI_IRQHandler(hallSensor_Pin)` → `freertos.c` içindeki `HAL_GPIO_EXTI_Callback()`. |
| `TIM6_DAC_IRQHandler()` | TIM6 zaman tabanı kesmesini işler. | `HAL_TIM_IRQHandler()` → `main.c` içindeki `HAL_TIM_PeriodElapsedCallback()` → `HAL_IncTick()`. |
| `OTG_FS_IRQHandler()` | USB donanım kesmesini HAL'e iletir. | `HAL_PCD_IRQHandler()` üzerinden USB veri akışının ilerlemesini sağlar. |
| `HardFault_Handler()` | İşlemci hata durumunda burada kalır. | Debugger ile çağrı zincirini ve hata öncesi durumu incele; normal akışta buraya düşülmez. |

Bu dosyada enkoderin konum hesabını arama; burada olay yönlendirilir, uygulama hesabı `freertos.c` içindedir.

<a id="msp-timebase"></a>
### MSP ve HAL zaman tabanı — Çevrebirimlerin altyapısı

[stm32f4xx_hal_msp.c](../Core/Src/stm32f4xx_hal_msp.c) içinde şu bölümlere bak:

| Bölüm | Ne yapıyor? | Kontrol noktası |
| --- | --- | --- |
| `HAL_I2C_MspInit()` | I2C1 saatini ve PB6/PB9 alternatif fonksiyonlarını kurar. | `Audio_SCL/SDA` isimleri bu projede I2C pinleri için kullanılıyor. Eski isimleri işlevle karıştırma. |
| `HAL_SPI_MspInit()` | SPI1 saatini ve pinlerini hazırlar. | `main.c` içindeki `MX_SPI1_Init()` ile birlikte oku. |
| `HAL_TIM_Base_MspInit()` | TIM2 çevrebirim saatini açar. | Sayaç ayarı `MX_TIM2_Init()`, sayacı başlatma ise `main()` içindedir. |

[stm32f4xx_hal_timebase_tim.c](../Core/Src/stm32f4xx_hal_timebase_tim.c) içindeki `HAL_InitTick()` TIM6'yı HAL'in 1 ms zaman tabanı olarak kurar. `HAL_SuspendTick()` / `HAL_ResumeTick()` bu kesmeyi kapatır/açar. Gecikme veya zaman ölçümü sorunu incelerken **TIM2 mikro saniye sayacı**, **TIM6 HAL tick** ve **FreeRTOS tick** ayrımını koru.

<a id="usb"></a>
### USB dosyaları — Bilgisayarla veri alışverişi

[usb_device.c](../USB_DEVICE/App/usb_device.c) içindeki `MX_USB_DEVICE_Init()` USB cihazını, CDC sınıfını ve arayüzünü kurup başlatır. `main()` bu fonksiyonu çağırır.

[usbd_cdc_if.c](../USB_DEVICE/App/usbd_cdc_if.c) içinde şu bölümlere bak:

| Bölüm | Ne yapıyor? | Neye dikkat etmelisin? |
| --- | --- | --- |
| `CDC_Init_FS()` | Gönderme/alma tamponlarını bağlar. | USB haberleşmesinin başlangıç kurulumudur. |
| `CDC_Transmit_FS()` | Veriyi bilgisayara gönderme isteği oluşturur. | USB sınıfı hazır değilse `USBD_FAIL`, önceki aktarım sürüyorsa `USBD_BUSY` döner. Çağıran görev bu sonucu değerlendirmelidir. |
| `CDC_Receive_FS()` | Bilgisayardan gelen veriyi `TerminalKomutAl()` fonksiyonuna iletir ve alımı yeniden hazırlar. | **Mevcut `TerminalKomutAl()` gövdesi boştur; gelen komutlar işlenmez.** Yorumlar aktif terminal kontrolü varmış gibi okunmamalı. |

**Gönderim zinciri:** `StartTelemetryTask()` metni hazırlar → `CDC_Transmit_FS()` USB'ye verir → bilgisayar seri porttan okur. Mesajın alanlarını değiştirmek için öncelikle `freertos.c` içindeki `snprintf()` bölümüne bak.

<a id="madgwick"></a>
### madgwick.c / madgwick.h — Henüz görev akışına bağlı olmayan filtre

[Kodu aç](../Core/Src/madgwick.c) · [Arayüz ve birimleri aç](../Core/Inc/madgwick.h).

| Bölüm | Ne yapıyor? | Neye dikkat etmelisin? |
| --- | --- | --- |
| `Madgwick_Init()` | Filtre durumunu ve beta kazancını başlatır. | IMU desteği eklenirken başlangıç akışına bağlanması gerekir. |
| `Madgwick_Update()` | Jiroskop, ivmeölçer, manyetometre verisi ve zaman adımıyla yönelim hesabını günceller. | Başlıktaki birimler: rad/s, g, µT ve saniye. Geçersiz/sıfır sensör vektörleri ve `dt` entegrasyon sırasında kontrol edilmeli. |
| `Madgwick_GetEuler()` | Roll/pitch/yaw açılarını derece cinsinden verir. | Filtrenin kaynakta bulunması, bu değerlerin şu anda hesaplanıp telemetriye eklendiği anlamına gelmez. |

İlk incelemede bu dosyayı sonraya bırakabilirsin. Aktif `StartIMUTask()` sadece bekliyor; sensör okuma ve filtre çağrıları henüz o göreve bağlanmamış.

<a id="project-config"></a>
### Proje ayarları — Kod dışındaki önemli dosyalar

| Dosya | Bakılacak yer | Neden önemli? |
| --- | --- | --- |
| [CubeMX yapılandırması](../partech_hyperloop_navigasyon10.ioc) | MCU, pinler, clock, FreeRTOS görevleri ve firmware paket sürümü | Donanım tasarım ayarlarını gösterir. Yeniden kod üretmeden önce commit al ve sonrasında diff incele. |
| [.cproject](../.cproject) | Debug/Release, include yolları, MCU ve linker script seçimi | CubeIDE'nin projeyi nasıl derleyeceğini belirler. Ayar değişikliklerini IDE üzerinden yapmak daha kolaydır. |
| [.project](../.project) | Proje adı ve proje türleri | IDE'de neden `partech_hyperloop_navigasyon10` adının göründüğünü açıklar. |
| [FLASH linker betiği](../STM32F407VGTX_FLASH.ld) | `MEMORY` ve `SECTIONS` | Programın Flash/RAM yerleşimini belirler. Heap/stack büyütmeden önce toplam RAM kullanımını kontrol et. |
| [RAM linker betiği](../STM32F407VGTX_RAM.ld) | `MEMORY` ve `SECTIONS` | Alternatif RAM yerleşimidir; mevcut Debug/Release ayarları FLASH betiğini seçer. |

Değişiklikleri ayrı Git dalında yap. CubeMX kod üretiminden önce commit al; sonrasında özellikle main.c, freertos.c, GPIO ve USB başlangıç değişikliklerini karşılaştır.
