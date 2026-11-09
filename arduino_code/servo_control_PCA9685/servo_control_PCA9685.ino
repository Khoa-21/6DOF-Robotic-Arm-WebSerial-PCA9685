/*
 * ========================================================
 *   ĐIỀU KHIỂN ROBOT PCA9685 (CÓ CHỈNH TỐC ĐỘ & LƯỚT MƯỢT)
 * ========================================================
 * 
 * Tính năng:
 *   1. Chỉnh tốc độ thực tế từ thanh "⚡ Tốc độ" trên Web (1-100%):
 *      - Tốc độ 100%: Đi nhanh tức thì theo tay kéo.
 *      - Tốc độ 50-80%: Lướt êm ái, chống khựng giật, triệt tiêu quán tính.
 *      - Tốc độ 10-30%: Chuyển động chậm rãi, mượt mà.
 *   2. Đã cân chỉnh Khớp Vai (đảo chiều thuận).
 *   3. Giới hạn Tay Kẹp tối đa 123° (chống kẹt nhông).
 *   4. Đứng yên khi cắm nguồn (không tự nhảy góc).
 *   5. Chuẩn tốc độ 9600 Baud khớp hoàn toàn với Web.
 *
 * Thứ tự kênh PCA9685:
 *   - Thanh 1: Đế Xoay     -> Kênh 5 trên PCA9685
 *   - Thanh 2: Khớp Vai    -> Kênh 4 trên PCA9685 (Đã đảo chiều)
 *   - Thanh 3: Khớp Khuỷu  -> Kênh 3 trên PCA9685
 *   - Thanh 4: Cổ Tay Xoay -> Kênh 2 trên PCA9685
 *   - Thanh 5: Cổ Tay Gập  -> Kênh 1 trên PCA9685
 *   - Thanh 6: Tay Kẹp     -> Kênh 0 trên PCA9685 (0 - 123°)
 * ========================================================
 */

#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40);

#define SO_KENH 6
#define BAUD_RATE 9600

// Ánh xạ thứ tự kênh PCA9685 (Thanh 1..6 -> PCA 5, 4, 3, 2, 1, 0)
const int kenhPCA[SO_KENH] = {5, 4, 3, 2, 1, 0};

// Cấu hình đảo chiều (Khớp Vai đảo chiều)
const bool daoChieu[SO_KENH] = {false, true, false, false, false, false};

// Giới hạn góc tối đa an toàn:
// Khớp Vai giới hạn tối đa 140 độ để chống kẹt cơ học / rung lắc ở 145-147 độ!
// Tay Kẹp tối đa 123 độ chống kẹt nhông
const int gocMax[SO_KENH] = {180, 140, 180, 180, 180, 123};

// Độ rộng xung min - max (micro-giây) cho từng kênh:
// Riêng Cổ Tay Xoay (Index 3 - PCA 2): hạ xung min xuống 400us để quay hết hành trình 0 độ
const int servoMinUs[SO_KENH] = {500, 500, 500, 400, 500, 500};
const int servoMaxUs[SO_KENH] = {2500, 2500, 2500, 2600, 2500, 2500};

// Các biến quản lý góc và tốc độ
int gocHienTai[SO_KENH];
int gocMucTieu[SO_KENH];
bool daKhoiDong[SO_KENH];

// 3 MỨC TỐC ĐỘ:
//   1 = CHẬM (20ms/độ ~50°/s, siêu êm, chống sụt áp và triệt tiêu rung giật)
//   2 = VỪA  (8ms/độ  ~125°/s, đầm tay, lướt mượt chuẩn xác)
//   3 = NHANH (0ms tức thì, bám sát tay kéo tối đa công suất)
int cheDoTocDo = 2; // Mặc định: Vừa
unsigned long thoiGianTruoc = 0;
String boNhoSerial = "";

int usToTicks(int pulseUs) {
  return (long)pulseUs * 4096 / 20000;
}

void datGocPca(int idx, int angle) {
  angle = constrain(angle, 0, 180);
  int pulse = map(angle, 0, 180, servoMinUs[idx], servoMaxUs[idx]);
  int ticks = usToTicks(pulse);

  int channel = kenhPCA[idx];
  // Kỹ thuật Lệch Pha PWM (Phase Staggering):
  int onTick = channel * 250;
  pwm.setPWM(channel, onTick, onTick + ticks);
}

void setup() {
  Serial.begin(BAUD_RATE);
  boNhoSerial.reserve(32);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  // Khởi động I2C chuẩn tốc độ cao (400kHz Fast Mode) giúp giao tiếp siêu tốc, không bị lag vi điều khiển
  Wire.begin();
  Wire.setClock(400000);
  pwm.begin();
  pwm.setPWMFreq(50);
  delay(100);

  // TẮT XUNG PWM TẤT CẢ 16 KÊNH ĐỂ ĐỨNG YÊN TUYỆT ĐỐI KHI VỪA CẮM NGUỒN
  for (int ch = 0; ch < 16; ch++) {
    pwm.setPWM(ch, 0, 4096);
  }

  for (int i = 0; i < SO_KENH; i++) {
    gocHienTai[i] = 90;
    gocMucTieu[i] = 90;
    daKhoiDong[i] = false;
  }

  digitalWrite(LED_BUILTIN, LOW);
  Serial.println(F("PCA9685_OK"));
}

void loop() {
  // 1. Nhận lệnh từ Web Serial
  while (Serial.available() > 0) {
    char c = Serial.read();

    if (c == '\n' || c == '\r') {
      if (boNhoSerial.length() > 0) {
        xuLyLenh(boNhoSerial);
        boNhoSerial = "";
      }
    } else {
      boNhoSerial += c;
      if (boNhoSerial.length() > 25) {
        boNhoSerial = "";
      }
    }
  }

  // 2. Cập nhật chuyển động theo tốc độ điều chỉnh
  capNhatTheoTocDo();
}

void xuLyLenh(String lenh) {
  lenh.trim();
  if (lenh.length() == 0) return;

  // LỆNH ĐIỀU CHỈNH TỐC ĐỘ: SPD:CHAM, SPD:TB, SPD:NHANH (hoặc SPD:1, SPD:2, SPD:3)
  if (lenh.startsWith("SPD:") || lenh.startsWith("spd:")) {
    String val = lenh.substring(4);
    val.trim();
    if (val.equalsIgnoreCase("CHAM") || val == "1") {
      cheDoTocDo = 1;
      Serial.println(F("OK_SPD:CHAM"));
    } else if (val.equalsIgnoreCase("TB") || val.equalsIgnoreCase("VUA") || val == "2") {
      cheDoTocDo = 2;
      Serial.println(F("OK_SPD:TB"));
    } else if (val.equalsIgnoreCase("NHANH") || val == "3") {
      cheDoTocDo = 3;
      Serial.println(F("OK_SPD:NHANH"));
    } else {
      int s = val.toInt();
      if (s <= 35) { cheDoTocDo = 1; Serial.println(F("OK_SPD:CHAM")); }
      else if (s <= 80) { cheDoTocDo = 2; Serial.println(F("OK_SPD:TB")); }
      else { cheDoTocDo = 3; Serial.println(F("OK_SPD:NHANH")); }
    }
    return;
  }

  int kenh = -1;
  int goc = -1;

  // Lệnh web: S1:090
  if ((lenh.charAt(0) == 'S' || lenh.charAt(0) == 's') && lenh.indexOf(':') > 0) {
    int viTri = lenh.indexOf(':');
    kenh = lenh.substring(1, viTri).toInt();
    goc = lenh.substring(viTri + 1).toInt();
  } 
  // Lệnh cũ: s190
  else if ((lenh.charAt(0) == 'S' || lenh.charAt(0) == 's') && lenh.length() >= 3) {
    kenh = lenh.substring(1, 2).toInt();
    goc = lenh.substring(2).toInt();
  }

  if (kenh >= 1 && kenh <= SO_KENH) {
    int idx = kenh - 1;

    // Giới hạn góc an toàn (Tay kẹp tối đa 123 độ)
    goc = constrain(goc, 0, gocMax[idx]);

    // Đảo chiều Khớp Vai
    int gocThucTe = goc;
    if (daoChieu[idx]) {
      gocThucTe = 180 - goc;
    }

    gocMucTieu[idx] = gocThucTe;

    // Lần đầu nhận lệnh từ kênh này thì kích hoạt góc
    if (!daKhoiDong[idx]) {
      gocHienTai[idx] = gocThucTe;
      datGocPca(idx, gocThucTe);
      daKhoiDong[idx] = true;
    }

    // Nháy LED 13
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  }
}

// BỘ ĐIỀU KHIỂN 3 MỨC TỐC ĐỘ THỰC TẾ
void capNhatTheoTocDo() {
  // MỨC 3 (NHANH): Quay ngay lập tức theo tay kéo (tức thì 0ms)
  if (cheDoTocDo == 3) {
    for (int i = 0; i < SO_KENH; i++) {
      if (daKhoiDong[i] && gocHienTai[i] != gocMucTieu[i]) {
        gocHienTai[i] = gocMucTieu[i];
        datGocPca(i, gocHienTai[i]);
      }
    }
    return;
  }

  // MỨC 1 (CHẬM): delay 20ms/độ (~50°/s) - Rất êm ái, chống rung, triệt tiêu sụt áp
  // MỨC 2 (VỪA):  delay 8ms/độ  (~125°/s) - Di chuyển nhanh vừa phải, đầm tay, lướt mượt
  unsigned long delayBuoc = (cheDoTocDo == 1) ? 20 : 8;
  unsigned long now = millis();

  if (now - thoiGianTruoc >= delayBuoc) {
    thoiGianTruoc = now;

    for (int i = 0; i < SO_KENH; i++) {
      if (!daKhoiDong[i]) continue;

      if (gocHienTai[i] < gocMucTieu[i]) {
        gocHienTai[i]++;
        datGocPca(i, gocHienTai[i]);
      } else if (gocHienTai[i] > gocMucTieu[i]) {
        gocHienTai[i]--;
        datGocPca(i, gocHienTai[i]);
      }
    }
  }
}
