# Doğrulama durumu ve bilinen noktalar

## 18 Ekim 2026 aktarımı

Uygulama ve kütüphane dosyaları orijinal projeden korunarak aktarıldı. Eski `Debug/`, `.DS_Store` ve kişisel `.launch` dosyaları dışarıda bırakıldı. Türkçe belgeler ve `.gitignore` eklendi. `.cproject` içindeki artık mevcut olmayan USB_HOST include yolları temizlendi; uygulama kaynakları değiştirilmedi.

Kaynak dosyaların kopyayla eşitliği, proje XML'lerinin okunabilirliği, kaynak/include yolları ve yerel belge bağlantıları kontrol edildi. Bu kontroller C derlemesi veya donanım testi değildir. Aktarım ortamında STM32CubeIDE ve `arm-none-eabi-gcc` bulunmadığı için Debug/Release derlemesi, flash ve çalışma testi yapılmadı.

## İlk çalıştırmadan önce incelenecek noktalar

1. **RTOS heap bütçesi:** Beş uygulama görevinin stack toplamı 14336 bayt; `configTOTAL_HEAP_SIZE` 15360 bayt. Dinamik görev kontrol blokları, newlib reentrancy alanları ve RTOS nesneleri için az alan kalır. `osThreadNew()` sonuçları mevcut kodda kontrol edilmiyor. Beş tutamacı ve scheduler başlangıcını doğrulayın; gerekirse linker RAM bütçesiyle birlikte heap'i artırın. Bu bir statik inceleme bulgusudur; tahsis başarısızlığı donanımda ölçülmedi.
2. **Fren 1 başlangıç portu:** `main.h` fren 1'i PD0 olarak tanımlar. `MX_GPIO_Init()` içindeki ilk seviye yazımı ise `HAL_GPIO_WritePin(GPIOE, motor_ileri_Pin|fren_cikis1_Pin, GPIO_PIN_SET)` kullanıyor; fren bitini PE0'a yazar. Sonraki GPIO init PD0'ı kullanır. İlk açılış seviyesi ile gerçek fren devresi eşleştirilerek düzeltilmeli/test edilmelidir. Bu aktarımda mevcut davranış korunmuştur.
3. **IMU eksik:** IMUTask yalnızca bekler. IMU verisi veya füzyonunun çalıştığı kabul edilmemelidir.
4. **CubeMX yeniden üretimi:** I2C kurtarma çağrısı ve özel başlangıç sırası main.c içinde korunan USER CODE alanlarının dışında da bulunuyor. Otomatik üretim bunları değiştirebilir. İlk derleme için kod üretmeyin; üretim gerekirse ayrı dalda diff inceleyin.
5. **Sensör ve pist sabitleri:** ROM kimlikleri, teker çevresi, PPR, reflektör haritası ve fren eşikleri donanıma özeldir.
6. **Stack izleme:** `vApplicationStackOverflowHook()` mevcut ancak FreeRTOSConfig.h içinde `configCHECK_FOR_STACK_OVERFLOW` etkinleştirilmemiş. Hook'un bulunması taşma izlemesinin aktif olduğunu göstermez.

## Ekip doğrulama kaydı

Aşağıdakiler henüz tamamlanmadı; test eden ekip üyesi sonucu ve commit'i eklemelidir.

- [ ] Debug temiz derleme: IDE ve GCC sürümü, hata/uyarı özeti
- [ ] Release temiz derleme
- [ ] Beş görev oluşturuluyor ve scheduler başlıyor
- [ ] Açılışta motor/fren pin seviyeleri ölçüldü
- [ ] USB CDC satırları ve alanları doğrulandı
- [ ] DS18B20 kimlikleri, CRC ve sıcaklıklar doğrulandı
- [ ] Omron/enkoder sayaçları ve kaynak geçişi doğrulandı
- [ ] Manuel/otonom kontrol kontrollü düzende test edildi

Başarılı derleme, gerçek hareket sisteminin doğru çalıştığını tek başına kanıtlamaz.
