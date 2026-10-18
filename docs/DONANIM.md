# Donanım ve uyarlama

Bu tablo kaynak koddaki eşlemeyi gösterir; fiziksel tesisatın doğrulandığı anlamına gelmez. Kart şeması ve harici sürücü bağlantıları bu projeyle verilmemiştir. Hedef yapılandırma STM32F407G-DISC1 / STM32F407VGT6, HSE 8 MHz'dir.

| İşlev | MCU pini | Kod davranışı |
| --- | --- | --- |
| Enkoder / hallSensor | PE3 | İki kenarlı EXTI |
| Omron Sensor1 / 2 / 3 | PB11 / PB13 / PB15 | Aktif düşük giriş, filtreli |
| İleri / geri isteği | PE7 / PE9 | Aktif yüksek |
| Fren girişleri 1 / 2 | PE5 / PC13 | Aktif yüksek |
| Otonom istek | PC9 | Aktif yüksek |
| Yazılım reset isteği | PA3 | Aktif yüksek |
| Motor ileri / geri | PE13 / PD9 | Aktif düşük çıkış |
| Fren çıkışları 1 / 2 | PD0 / PD2 | Aktif düşük, open-drain |
| s1 | PD8 | İleri/geri yardımcı çıkışı |
| Canlılık sinyali | PD11 | ControlTask başlangıcında yüksek |
| DS18B20 ortak veri hattı | PA1 | 1-Wire open-drain |
| I2C1 SCL / SDA | PB6 / PB9 | I2C hattı |
| MPU9250_CS | PC1 | Başlangıçta yüksek |
| USB FS DM / DP | PA11 / PA12 | USB CDC |
| SWDIO / SWCLK | PA13 / PA14 | ST-LINK debug |

Başlıktaki pin adları bazı eski donanım adlarını içerir. `StartIMUTask()` boş olduğundan BNO055/MPU9250 isimlerinden aktif IMU desteği sonucu çıkarmayın.

## Yeni düzeneğe uyarlama

- `freertos.c` içindeki `sensor1_id`, `sensor2_id`, `sensor3_id` DS18B20 ROM kimlikleridir. Başlangıç taraması `Bulunan_IDler` ve `Bulunan_Sensor_Sayisi` değerlerini doldurur, fakat okuma sabit kimlikleri kullanır. Yeni sensörlerin kimliklerini debugger ile belirleyip eşleyin.
- `TEKER_CEVRESI_M=0.172788`, `ENCODER_PPR=400` mekanik düzeneğe özeldir.
- `MesafeHesapla()`, 186 m sınırı ve otonom fren eşikleri pist düzeniyle birlikte gözden geçirilmelidir.
- 1-Wire pull-up, sensör beslemesi ve harici sinyal seviyelerini gerçek devreye göre doğrulayın. Tablo elektriksel bağlantı şemasının yerini tutmaz.
- Motor/fren testinden önce aşağıdaki başlangıç GPIO tutarsızlığını inceleyin: [doğrulama notları](DOGRULAMA.md).
