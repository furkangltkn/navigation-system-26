# PARTECH — Navigation System 26

STM32F407 tabanlı navigasyon, motor/fren kontrolü ve USB telemetri firmware projesi. Yeni ekip üyeleri için kaynak kodlar, STM32CubeIDE proje ayarları ve kullanılan kütüphaneler bu repoda birlikte tutulur.

**Durum:** Orijinal `partech_hyperloop_navigasyon10` kaynaklarının aktarımıdır. Bu aktarımda uygulama davranışı değiştirilmedi. Dosya/yapı kontrolleri yapıldı; temiz CubeIDE derlemesi ve kart testi henüz yapılmadı. Özellikle [bilinen noktaları](docs/DOGRULAMA.md) ilk donanım çalıştırmasından önce okuyun.

## İlk kez bakıyorsan buradan başla

**Asıl uygulama kodları `Core/Src/` klasöründe.** Önce `main.c` ile sistemin nasıl başladığını, ardından `freertos.c` ile çalışırken ne yaptığını incele.

Bu yazılım STM32 kartına yüklenir. `.c` dosyaları tek başına çalıştırılmaz; CubeIDE bütün projeyi derleyip karta yüklenecek dosyayı oluşturur.

### Önerilen dosya okuma sırası

Dosya adına tıklayarak doğrudan kodu açabilirsin. Uzun dosyalarda belirtilen fonksiyon adını ara.

| Sıra | Açılacak dosya | Basitçe ne yapıyor? | İlk olarak nereye bakmalısın? |
| --- | --- | --- | --- |
| 1 | [main.c](Core/Src/main.c) | Kart açıldığında donanımı hazırlar ve görevleri başlatır. | `main()` fonksiyonu; ardından `osThreadNew()` çağrıları. |
| 2 | [freertos.c](Core/Src/freertos.c) | Hareket kontrolünü, navigasyonu, sıcaklık okumayı ve veri göndermeyi yürütür. **Ana iş mantığı burada.** | Aşağıdaki görev tablosunu takip et. |
| 3 | [main.h](Core/Inc/main.h) | Kodda kullanılan sinyal isimlerini STM32 pinleriyle eşleştirir. | `_Pin` ve `_GPIO_Port` tanımları. Örneğin `fren_cikis1` hangi pine bağlı? |
| 4 | [onewire.c](Core/Src/onewire.c) | DS18B20 sıcaklık sensörleriyle veri alışverişi yapar. | `OneWire_Init()`, `OneWire_Reset()` ve `OneWire_Search()`. |
| 5 | [FreeRTOSConfig.h](Core/Inc/FreeRTOSConfig.h) | Görevleri yöneten FreeRTOS sisteminin bellek ve zamanlama ayarlarını tutar. | `configTOTAL_HEAP_SIZE` ve `configTICK_RATE_HZ`. |

**Görev (task) nedir?** Belirli bir işi tekrar tekrar yapan fonksiyondur. FreeRTOS, bu görevlerin işlemciyi hangi sırayla kullanacağını yönetir. Görevler `main.c` içinde oluşturulur; yaptıkları işler `freertos.c` içinde yazılıdır.

### freertos.c içinde hangi bölümleri okuyacağım?

| Sıra | Aranacak fonksiyon | Cevapladığı soru | İncelerken dikkat et |
| --- | --- | --- | --- |
| 1 | `StartControlTask()` | İleri, geri, fren ve otonom hareket nasıl yönetiliyor? | Girişlerin önceliği, manuel müdahale ve otonom durum geçişleri. |
| 2 | `StartNavigationTask()` | Konum ve hız nasıl hesaplanıyor? | Omron/enkoder seçimi (`yedek_mod`), `LX` konumu ve `SX` hızı. |
| 3 | `StartSensorTask()` | Üç sıcaklık sensörü nasıl okunuyor? | `sensor1_id`, `sensor2_id`, `sensor3_id` sabit kimlikleri. |
| 4 | `StartTelemetryTask()` | Bilgisayara hangi bilgiler gönderiliyor? | `snprintf()` içindeki alanlar ve `CDC_Transmit_FS()` çağrısı. |
| 5 | `StartIMUTask()` | IMU sensörü okunuyor mu? | **Bu görev şu anda yalnızca bekliyor; IMU okuması yok.** |

Sonra yardımcı fonksiyonlara geç: `MesafeHesapla()` reflektörlerin konumlarını tanımlar, `HAL_GPIO_EXTI_Callback()` enkoder sinyalinin kenarlarını işler, `OlcumuBaslat()` ölçüm değerlerini sıfırlar.

### Hangi konu için hangi rehberi açmalıyım?

| Yapmak istediğin | Açılacak rehber |
| --- | --- |
| Projeyi CubeIDE'de açmak, derlemek ve karta yüklemek | [Adım adım kurulum](docs/KURULUM.md) |
| Görevleri, başlangıç akışını ve USB mesajlarını anlamak | [Kod rehberi](docs/KOD_REHBERI.md) |
| Sensör bağlantılarını ve değiştirilmesi gereken donanım değerlerini görmek | [Pinler ve donanım](docs/DONANIM.md) |
| Mevcut sorunları ve henüz yapılmamış testleri görmek | [Doğrulama notları](docs/DOGRULAMA.md) |

### Karta yüklemeden önce özellikle oku

- **Fren çıkışı:** Başlangıç kodundaki fren 1 port yazımı ile `main.h` pin tanımı arasında tutarsızlık var.
- **Görev belleği:** FreeRTOS için ayrılan bellek dar; beş görevin de başarıyla oluşturulduğu kontrol edilmeli.
- **Donanıma özel değerler:** Sıcaklık sensörü kimlikleri, teker çevresi, enkoder PPR değeri, reflektör haritası ve fren eşikleri kendi düzeneklerinle eşleşmeli. PPR, bir turdaki darbe sayısıdır.
- **Yeniden kod üretimi:** CubeMX ile otomatik kod üretmek bazı özel başlangıç kodlarını değiştirebilir. İlk inceleme ve derleme için mevcut kaynakları kullan.

Ayrıntılar ve test listesi [doğrulama notlarında](docs/DOGRULAMA.md). Bu repo henüz kart üzerinde doğrulanmış bir sürüm olarak işaretlenmedi.

### İlk okumada hangi klasörleri sonraya bırakabilirim?

`Drivers/` ve `Middlewares/` çoğunlukla hazır STM32 ve FreeRTOS kütüphaneleridir. İlk incelemede yukarıdaki uygulama dosyalarına odaklanabilirsin; ancak bu kütüphaneler derleme için gereklidir, silinmemelidir.

## Hedef ve araçlar

| Özellik | Projede kayıtlı değer |
| --- | --- |
| MCU | STM32F407VGT6 / STM32F407VGTx |
| Kart yapılandırması | STM32F407G-DISC1 |
| Saat | HSE 8 MHz, SYSCLK 168 MHz |
| RTOS | FreeRTOS V10.3.1, CMSIS-RTOS V2 |
| Cube firmware paketi | STM32Cube FW_F4 V1.28.3 |
| CubeMX | `.ioc` içinde 6.15.0 |
| CubeIDE | Eski debug kaydında 1.19.0 izi var; doğrulanmış sürüm değildir |
| Derleyici | Eski makefile kaydında GNU Tools for STM32 13.3.rel1 |

## Klasörler

```text
Core/                 Uygulama, pin tanımları, kesmeler ve başlangıç kodu
Drivers/              STM32 HAL ve CMSIS
Middlewares/          FreeRTOS ve ST USB kütüphaneleri
USB_DEVICE/           USB CDC uygulaması
.settings/            Paylaşılan CubeIDE ayarları
.project, .cproject   Eclipse/CubeIDE proje tanımı
*.ioc                 CubeMX donanım yapılandırması
*.ld                  Flash/RAM linker betikleri
docs/                 Türkçe ekip rehberi
```

`Core/Src/main.c` başlangıcı ve görev oluşturmayı, `Core/Src/freertos.c` uygulama görevlerini içerir. Bunlar tek başına çalıştırılmaz; tüm proje derlenip STM32'ye yüklenir. GitHub repo adı ile IDE proje adı farklıdır: IDE'de `partech_hyperloop_navigasyon10` görünmesi normaldir.

Derleme çıktıları ve eski kişisel debug oturumları paylaşılmıyor. Kaynakta bulunan üçüncü taraf lisansları kendi klasörlerinde korunmuştur; bu aktarım ekip koduna yeni bir lisans atamaz.
