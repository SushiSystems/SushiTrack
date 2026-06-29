# SushiTrack — IMM (Interacting Multiple Model) Tasarım Dokümanı

Durum: TASLAK / tartışma · Tarih: 2026-06-29 · Hedef: `TrackFSM`'i kaldırıp
`enable_imm` ile sürülen heterojen bir IMM kestiriciyle değiştirmek.

Kararlaştırılmış yön (FINAL):
- **Model seti:** **4-model** — CV + CA + CT + **Stationary/Brownian**. CT nonlineer.
- **Nonlineer filtre:** **UKF** (sigma-noktaları), EKF değil — robustluk tercihi.
- **Aktivasyon:** **`enable_imm` YOK.** IMM `kalman_type=2` ile açılır (registry deseni:
  0=Linear, 1=NSA, 2=IMM). Factory/registry imzası değişmez.
- **Konfigürasyon:** IMM sigma/geçiş sabitleri **gömülü** tutulur (mevcut
  `KalmanFilter`'ın gömülü sabit felsefesiyle birebir; user-knob eklenmez).
  Böylece ConfigLoader/C-API/Python zincirine dokunulmaz. Tek yeni public yüzey
  `maneuver_probability` çıktısı.
- **Public API:** `kinematic_state` korunur (türetilmiş); yanına sürekli
  `maneuver_probability` eklenir.
- **Entegrasyon mekanizması:** "delta-reankraj" (§2.1) — Tracklet'in paylaşılan
  `mean_`'e dokunuşları (size-velocity freeze) IMM'in iç modlarına yansıtılır.

### 2.1 Delta-reankraj — entegrasyon linçpini
IStatePredictor sözleşmesinde kestirim (mean/cov) Tracklet'te yaşar, predictor'a
referansla geçer. IMM ise iç mod-state'lerini (x_j, P_j, μ_j) kendi içinde
saklamak zorunda — bunu per-track `kf_` unique_ptr sayesinde yapabilir. Tek
çatışma: `Tracklet::predict` non-Tracked track'lerde `mean_`'in boyut-hız
bileşenlerini sıfırlıyor (ByteTrack freeze), predictor'dan bağımsız.

Çözüm: IMM her `predict()/update()` girişinde gelen 8D `mean`'i son yazdığı
birleşik kestirimle karşılaştırır; `delta = gelen − son_birleşik` (8 paylaşılan
boyutta) her moda uniform uygulanır. Normalde delta=0 (no-op); Tracklet boyut-hızı
sıfırladığında bu düzeltme tüm modlara taşınır. Tracklet'te **hiç değişiklik
gerekmez**.

---

## 1. Motivasyon — FSM neden zayıf

`TrackFSM` hareket modelini değiştirmiyor. Yaptığı tek şey `predict()` öncesi
process-noise çarpanlarını (1.0–2.0x) ayarlamak. Yani manevra yapan hedefi daha
iyi *takip etmiyor*; sadece belirsizliği şişirip gate'i genişletiyor. Kök sorunlar:

1. **Tek model.** Gerçek bir sabit-ivme veya dönüş modeli yok; rejim yalnızca Q'yu
   ölçekliyor.
2. **Sert anahtarlama + histerezis gecikmesi** (`kStableFrames=2`, `kWarmupFrames=3`)
   → adaptasyon geç kalıyor, eşik ayarı (`fsm_stationary_vel_thresh`,
   `fsm_maneuver_accel_thresh`) kırılgan.
3. **Gürültülü ivme.** İvme, sınıflandırmaya çalıştığı sinyalin ta kendisi olan
   Kalman hızının sonlu farkıyla kestiriliyor.

IMM bunları olasılıksal, sürekli ve geçiş-matrisiyle yumuşatılmış bir çoklu-model
karışımıyla değiştirir. Mod olasılıkları, FSM'in sert kategorilerinin yerine geçen
sürekli rejim tahminini doğal olarak verir.

---

## 2. Mevcut mimariyle uyum ve kısıtlar

| Gerçek | Sonuç |
|---|---|
| `predictor` her tracklet için ayrı yaratılıyor (`kf_` unique_ptr) | IMM **içsel state tutabilir**; per-track yaşam döngüsü hazır. |
| Registry int `kalman_type` ile çalışıyor (0=Linear, 1=NSA) | IMM **type=2** olarak kaydedilir; `enable_imm` bunu seçer. |
| `IStatePredictor`: `initiate/predict/update/setNoiseMultipliers` | IMM bu arayüzü implemente eder; `setNoiseMultipliers` no-op olur (Q moda içkin). |
| Gating `IKalmanFilter`'da (`mahalanobis/getGateFlags`) | IMM gating'i **birleşik kestirim** üzerinden verir. |
| **`KalmanTypes` 8D'ye sabit** (`StateMean 1x8`, `StateCov 8x8`) | **CA (10D) ve CT (9D) bu tipleri kullanamaz.** IMM içsel olarak kendi geniş tiplerini kullanır, dışarıya yalnızca 8D birleşik kestirimi verir. CV modu `KalmanFilter`'ı yeniden kullanabilir; CA/CT için yeni içsel filtreler gerekir. |

**Tasarım tercihi:** IMM dışarıya `IStatePredictor` + gating arayüzünü 8D olarak
sunar (Tracklet hiç değişmez). İçeride mod-spesifik boyutlarda kendi
filtrelerini tutar ve birleşik kestirimi 8D'ye projeksiyonlayarak Tracklet'in
`mean_/cov_`'una yazar.

---

## 3. Durum gösterimleri (decoupled center/size)

bbox takibinde merkez dinamiği "ilginç" olandır; boyut (w,h) genelde
near-constant-velocity. Bu nedenle **merkez kinematiği moda-özgü, boyut
kinematiği tüm modlarda ortak CV** olarak ayrıştırıyoruz. Bu, mixing'i
basitleştirir (boyut bileşenleri tüm modlarda aynı yapı).

Ortak boyut alt-state'i (tüm modlarda): `[w, h, vw, vh]` — lineer CV.

Moda-özgü merkez alt-state'i:

| Mod | Merkez state | Toplam dim | Dinamik |
|---|---|---|---|
| **CV** | `[cx, cy, vcx, vcy]` | 8 | lineer |
| **CA** | `[cx, cy, vcx, vcy, acx, acy]` | 10 | lineer |
| **CT** | `[cx, cy, vcx, vcy, ω]` | 9 | **nonlineer** (vx,vy,ω kuplajı) |

Tam state vektörleri (sıralama: merkez-poz, boyut, merkez-hız, boyut-hız, ekstra):
```
CV:  [cx cy w h  vcx vcy vw vh]                 (8)
CA:  [cx cy w h  vcx vcy vw vh  acx acy]        (10)
CT:  [cx cy w h  vcx vcy vw vh  ω]              (9)
```

**İmplementasyon kolaylığı için ortak maksimum-boyut (10D) tampon** öneriyorum:
her mod 10D bir state/cov bloğunda yaşar, kullanılmayan bileşenler maskelenir
(CV için acx,acy,ω = 0 ve ilgili kovaryans satır/sütunları sıfır/devre dışı; CT'nin
ω'sı CA'nın acx slotunu kullanır). Bu, dinamik Eigen allokasyonundan kaçınır
(`Eigen::Matrix<float,10,10>` sabit boyut, SIMD-dostu).

### Ölçüm modeli (ortak)
Tüm modlar aynı H ile `[cx, cy, w, h]` (4D) ölçümüne map eder → **olabilirlikler
karşılaştırılabilir** (heterojen IMM için kritik). Ölçüm SushiTrack'taki mevcut
`DetectBox` (1x4).

---

## 4. Hareket modelleri

dt = adım süresi (time-aware'de gerçek dt, değilse 1).

### 4.1 CV (lineer)
Mevcut `KalmanFilter` ile birebir. Merkez: `cx += vcx·dt`. Process gürültüsü:
beyaz-ivme modeli, `σ_cv`.

### 4.2 CA (lineer)
`cx += vcx·dt + ½·acx·dt²`, `vcx += acx·dt`. Process gürültüsü: beyaz-jerk
modeli, `σ_ca` (jerk std). CA, manevranın *büyüklüğünü* taşır.

### 4.3 CT — Coordinated Turn (nonlineer, EKF)
Sabit dönüş hızı ω ile düzlemde dönüş:
```
cx⁺ = cx + (vx·sin(ωdt) − vy·(1−cos(ωdt)))/ω
cy⁺ = cy + (vx·(1−cos(ωdt)) + vy·sin(ωdt))/ω
vx⁺ = vx·cos(ωdt) − vy·sin(ωdt)
vy⁺ = vx·sin(ωdt) + vy·cos(ωdt)
ω⁺  = ω
```
ω→0 limitinde CV'ye indirgenir (sayısal olarak `|ω|<ε` için CV formülüne
düş). Predict adımı **EKF**: yukarıdaki f(x)'in analitik Jacobian'ı F = ∂f/∂x ile
kovaryans propagasyonu. Process gürültüsü: `σ_ct` (ivme) + `σ_ω` (dönüş-hızı
gürültüsü).

**Neden EKF, UKF değil:** CT Jacobian'ı kapalı-form ve ucuz; 9D sigma-nokta
propagasyonu (UKF) track başına belirgin daha pahalı. UKF ileride bir
alternatif olarak bırakılır (robustluk gerekirse).

Boyut alt-state'i (`w,h,vw,vh`) üç modda da aynı lineer CV bloğu — moddan
bağımsız propagasyon, sadece merkez/ekstra bileşenler farklı.

---

## 5. IMM döngüsü (4 adım)

Modlar j ∈ {CV, CA, CT}. Mod olasılıkları μ_j, geçiş matrisi Π (p_ij = i→j),
her mod için (mean x_j, cov P_j) ortak 10D uzayda.

**1) Etkileşim / karıştırma (mixing).**
```
c̄_j = Σ_i p_ij · μ_i                       (öngörülen mod olasılığı)
μ_{i|j} = p_ij · μ_i / c̄_j                  (karıştırma ağırlıkları)
x0_j = Σ_i μ_{i|j} · x_i
P0_j = Σ_i μ_{i|j} · (P_i + (x_i−x0_j)(x_i−x0_j)ᵀ)
```
**Heterojen boyut yönetimi (uygulanan yaklaşım):** Maskeleme yerine ortak uzay
**11D**'ye genişletilir: `[cx cy w h vx vy vw vh ax ay ω]`. Slot semantiği tüm
modlarda **tutarlıdır** (slot 8=ax her zaman, slot 10=ω her zaman). Her mod sahip
olmadığı genişletilmiş bileşeni transition'da sıfıra sabitler (CV/NCP: ax,ay,ω;
CA: ω; CT: ax,ay) ve process-noise'da tiny tutar. Böylece mixing ve combine
**model-agnostik** kalır — özel maske mantığı gerekmez. Sahip-olunmayan bileşen
mixing'e ~0 değer + sınırlı kovaryansla girer; bu, "uninformatif prior"un doğru
karşılığıdır.

**2) Mod-eşleşmeli predict + update.** Her mod x0_j,P0_j'den başlayıp kendi
modeliyle predict eder; ölçüm z varsa update eder ve olabilirliği üretir:
```
Λ_j = N(ν_j; 0, S_j)        ν_j: innovation, S_j: innovation kovaryansı
```
CT için predict EKF; CV/CA lineer.

**3) Mod olasılığı güncelleme.**
```
μ_j = Λ_j · c̄_j / Σ_k (Λ_k · c̄_k)
```

**4) Birleşim (çıktı).**
```
x = Σ_j μ_j · x_j
P = Σ_j μ_j · (P_j + (x_j−x)(x_j−x)ᵀ)
```
x, P → 8D'ye projeksiyon (acx,acy,ω düşürülür) → Tracklet `mean_/cov_`.

**Gözlemsiz kareler (Lost/Coasting):** adım 2'de update atlanır (sadece predict),
adım 3'te olasılıklar ya korunur ya da öngörülen c̄_j'ye gevşetilir. Bu, FSM'in
Coasting davranışının olasılıksal karşılığıdır.

---

## 6. Mod olasılıkları → rejim sinyalleri

FSM'in tüm tüketicileri (bkz. §8) IMM'den türetilmiş sinyallerle beslenir:

- **`maneuver_probability` = μ_CA + μ_CT** (manevra modellerine düşen kütle).
  Sürekli [0,1] sinyal — yeni public API alanı.
- **Türetilmiş `KinematicState`** (geri uyumluluk için):
  - birleşik hız < eşik → `Stationary`
  - aksi halde `maneuver_probability` > τ → `Maneuvering`
  - değilse → `Cruising`
  - track Lost iken → `Coasting` (mevcut semantik korunur)

Böylece hem mevcut `KinematicState` kontratı bozulmadan kalır, hem de iç
tüketiciler isterse ham sürekli olasılığı kullanabilir.

---

## 7. Sınıf / arayüz tasarımı

```
IStatePredictor (mevcut, değişmez)
   └── IMMFilter : IKalmanFilter           // type=2, dışarıya 8D
         - std::array<ModeFilter, K> modes_  // CV, CA, CT
         - Eigen::Vector3f mode_probs_
         - Eigen::Matrix3f transition_       // Π
         - 10D ortak tampon + aktif-bileşen maskeleri
         + initiate/predict/update           // §5 döngüsü
         + mahalanobis/getGateFlags          // birleşik kestirim üzerinden
         + setNoiseMultipliers → no-op
         + maneuverProbability() const
         + derivedKinematicState() const

ModeFilter (içsel, IMM-private)
   - virtual predict(x,P,dt)  // CV/CA lineer, CT EKF (Jacobian)
   - measurement H ortak
   - σ parametreleri moda özgü
```

CV mod-filtresi mevcut `KalmanFilter` matematiğini yeniden kullanabilir; CA/CT
yeni içsel implementasyonlar. Ortak boyut-bloğu propagasyonu paylaşılır.

---

## 8. FSM tüketicilerinin migrasyonu (yük taşıyan!)

FSM beklenenden bağlı. Hepsine IMM karşılığı şart:

| Tüketici | Yer | Migrasyon |
|---|---|---|
| Lost-buffer ölçekleme | `SushiTrackTracker.cpp:852` | `getLastSeenState` → türetilmiş `KinematicState` (veya `maneuver_probability` ile sürekli ölçek). |
| Association cost mods | `DistanceStrategies.cpp:646/891/1205` `stateAssocMods()` | türetilmiş rejim; ya da sürekli olasılıkla gate-scale/cost-bias enterpolasyonu. |
| OCM ağırlığı | `DistanceStrategies.cpp:1078` | Cruising/Maneuvering ayrımı → `maneuver_probability` ile sürekli ağırlık (mevcut `ocm_maneuver_weight_scale` doğal genelleşir). |
| **Public output** | `SushiTrackC.cpp:520`, `sushitrack.py:75` | `kinematic_state` korunur (türetilmiş) + yeni `maneuver_probability` alanı C struct + Python dataclass'a eklenir. |

**Decoupling önerisi:** Tüketicilerin `TrackFSM`'e doğrudan bağımlılığını bir
küçük rejim-sağlayıcı soyutlamasının (Tracklet üzerinde `getManeuverProbability()`
+ `getKinematicState()`) arkasına alalım; böylece FSM ve IMM aynı kontratı
besler, geçiş risksiz olur.

---

## 9. Konfigürasyon

Eklenecek:
```
bool  enable_imm        = false;   // type=2 IMM kestiriciyi seçer
float imm_sigma_cv      = ...;     // CV process (beyaz-ivme) std
float imm_sigma_ca      = ...;     // CA process (beyaz-jerk) std
float imm_sigma_ct      = ...;     // CT ivme std
float imm_sigma_omega   = ...;     // CT dönüş-hızı gürültü std
float imm_p_stay        = 0.90f;   // Π diyagonali (modda kalma olasılığı)
// başlangıç mod olasılıkları: CV ağırlıklı, ör. [0.8, 0.1, 0.1]
float imm_maneuver_thresh = 0.5f;  // türetilmiş KinematicState için τ
```
Π, `imm_p_stay` ve off-diagonal eşit dağıtımdan üretilir (3x3); ileride tam
matris açılabilir.

Kaldırılacak (Faz 4): `enable_fsm`, `fsm_stationary_vel_thresh`,
`fsm_maneuver_accel_thresh`, `fsm_reference_height`.

---

## 10. Fazlandırma (durum)

- **Faz 0 ✅:** İçsel tipler, `IMMFilter` iskeleti, registry **type=2**, derlenebilir
  CV-delege sürüm. (Not: `enable_imm` yerine `kalman_type=2` benimsendi.)
- **Faz 1 ✅:** Lineer **CV + CA + NCP** 3-model IMM; tam mixing→predict→update→
  olabilirlik→olasılık→birleşim döngüsü + delta-reankraj.
- **Faz 2 ✅:** **CT + UKF** dördüncü model (11D tutarlı-slot); ω→0 CV limiti.
- **Faz 3 ✅:** `maneuver_probability` + türetilmiş `KinematicState` Tracklet
  sınırından association / lost-buffer / OCM / public API'ye bağlandı.
- **Faz 4 ✅:** `TrackFSM`, `fsm_*` config, `TestTrackFSM` kaldırıldı; `KinematicState`
  nötr başlığa taşındı; testler/JSON/Python/CLI/README temizlendi.

Doğrulama: A/B karşılaştırması `kalman_type` 0/1 (tek-model) vs `2` (IMM) ile
dancetrack0002 üzerinde (önceki FSM+OCM baseline IDSW 68).

---

## 11. Riskler

- **8D tip sabitlemesi** kod tabanında yaygın; IMM içsel geniş tiplerle izole
  edilmeli, sızıntı olmamalı.
- **Maliyet:** track başına 3 filtre × predict/update + CT Jacobian. Track sayısı
  düşük olduğundan kabul edilebilir; sabit-boyut Eigen ile SIMD.
- **Gating** ideal olarak karışım innovation kovaryansıyla; ilk sürümde birleşik
  kestirimle yaklaşılır.
- **Sayısal:** mixing ve birleşim adımlarında kovaryans PSD korunmalı (simetri
  zorlama + jitter).
- **Heterojen mixing** en hata-yapılabilir kısım; gözlemlenemez bileşen maskesi
  birim testlerle sıkı doğrulanmalı.
