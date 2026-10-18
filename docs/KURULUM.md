# CubeIDE ile kurulum ve çalıştırma

## 1. Repoyu edin

```sh
git clone https://github.com/furkangltkn/navigation-system-26.git
```

Git kullanmıyorsanız GitHub'dan Code → Download ZIP ile indirip arşivi çıkarın. `.project` ve `.cproject` gibi noktayla başlayan dosyaları silmeyin.

## 2. Mevcut projeyi içe aktar

STM32CubeIDE'nin Eclipse tabanlı sürümünü açın, repo dışında bir workspace seçin. File → Import → General → Existing Projects into Workspace yolundan repo kökünü seçin. `partech_hyperloop_navigasyon10` projesini işaretleyip bitirin. Git ile aynı dosyalar üzerinde çalışmak için “Copy projects into workspace” kapalı kalabilir.

İlk açılışta `.ioc` üzerinden yeni proje üretmeyin. Bu depoda hazır kaynaklar vardır; otomatik yeniden üretim özel kodları değiştirebilir. IDE sürümüne göre menü adları değişebilir. İçe aktarma ve derleme akışı için [ST kullanıcı kılavuzu UM2609](https://www.st.com/resource/en/user_manual/um2609-stm32cubeide-user-guide-stmicroelectronics.pdf) temel alınmıştır.

## 3. Derle

Projenin etkin yapılandırmasını Debug yapın (Build Configurations → Set Active). Project → Clean, ardından Build Project çalıştırın. IDE kendi makefile'larını oluşturur; başka bilgisayardaki `Debug/` klasörüne ihtiyaç yoktur.

Başarılı derlemenin beklenen çıktısı `Debug/partech_hyperloop_navigasyon10.elf` dosyasıdır. Build Console'daki hataları kontrol edin; dosyanın tek başına varlığı yeni derlemenin başarılı olduğunu kanıtlamaz.

Derleyici bulunamazsa CubeIDE'nin ARM GNU toolchain kurulumunu kontrol edin. Paket istenirse `.ioc` içindeki STM32Cube FW_F4 V1.28.3 referansını esas alın. HAL/RTOS kaynakları repoda bulunur. Linker ayarı `${workspace_loc:/${ProjName}/STM32F407VGTX_FLASH.ld}` olarak projeye göre çözülür.

## 4. ST-LINK ile yükle

Önce [donanım eşlemesini](DONANIM.md) ve [açık doğrulama maddelerini](DOGRULAMA.md) kontrol edin. Kartı ST-LINK üzerinden bağlayın. Run → Debug As → STM32 C/C++ Application ile kendi debug yapılandırmanızı oluşturun; hedef STM32F407VGTx, arayüz SWD ve bu derlemede oluşan ELF olmalıdır. Debug oturumu `main()` içinde durursa Resume ile devam edin.

ST-LINK programlama bağlantısı ile uygulamanın USB CDC bağlantısı farklı işlevlerdir. Telemetri için MCU'nun USB OTG FS bağlantısını da bilgisayara bağlayın.

## 5. İlk masaüstü testi

Motor/fren güç katını devreden ayırarak başlayın; bu yazılım doğrudan hareket çıkışlarını sürer. Önce beş görev tutamacının da NULL olmadığını, scheduler'ın başladığını ve GPIO çıkışlarının beklenen seviyede olduğunu debugger ile gözleyin. USB CDC seri portunu açıp satırların yaklaşık 500 ms aralıklarla geldiğini kontrol edin. Sensör girişlerini kontrollü olarak uygulayın; `RC1/2/3`, `LX`, `SE` ve sıcaklıkları izleyin.

Kart üzerinde doğruladıktan sonra kullandığınız IDE/derleyici sürümünü, commit kimliğini, kartı ve test sonuçlarını `DOGRULAMA.md` içine ekleyin.

## Sık karşılaşılan durumlar

- Proje görünmüyor: `.project` bulunan kökü seçtiğinizden ve aynı adlı projenin workspace'te zaten bulunmadığından emin olun.
- IDE hatayı gösteriyor ama derleme geçiyor: index'i yeniden oluşturun; esas sonuç Build Console'dur.
- USB görünmüyor: veri taşıyan kablo, doğru USB portu ve `MX_USB_DEVICE_Init()` çağrısını kontrol edin.
- Sıcaklık -100: okuma/CRC başarısız olabilir; sabit ROM kimliklerini ve kablolamayı kontrol edin.
- Bazı görevler çalışmıyor: RTOS heap ve `osThreadNew()` dönüşlerini kontrol edin; ayrıntılar doğrulama belgesinde.
