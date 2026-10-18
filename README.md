# PARTECH — Navigation System 26

STM32F407 tabanlı navigasyon, motor/fren kontrolü ve USB telemetri firmware projesi. Yeni ekip üyeleri için kaynak kodlar, STM32CubeIDE proje ayarları ve kullanılan kütüphaneler bu repoda birlikte tutulur.

**Durum:** Orijinal `partech_hyperloop_navigasyon10` kaynaklarının aktarımıdır. Bu aktarımda uygulama davranışı değiştirilmedi. Dosya/yapı kontrolleri yapıldı; temiz CubeIDE derlemesi ve kart testi henüz yapılmadı. Özellikle [bilinen noktaları](docs/DOGRULAMA.md) ilk donanım çalıştırmasından önce okuyun.

## Nereden başlamalıyım?

1. [Kurulum, derleme ve karta yükleme](docs/KURULUM.md)
2. [Kod akışı, görevler ve telemetri](docs/KOD_REHBERI.md)
3. [Pinler ve donanıma uyarlama](docs/DONANIM.md)
4. [Doğrulama durumu ve bilinen noktalar](docs/DOGRULAMA.md)

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
