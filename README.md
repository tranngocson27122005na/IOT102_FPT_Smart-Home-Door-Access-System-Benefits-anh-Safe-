## Smart Home Door Access System Benefits anh Safe
## → Hệ thống truy cập cửa nhà thông minh, an toàn, tiện ích

Một bộ điều khiển cửa nhà thông minh dùng ESP32: cho phép mở/khóa cửa bằng thẻ RFID, điều khiển từ điện thoại qua Wi-Fi, và tự động mở khi sensor phát hiện người đến gần; trạng thái hiển thị trên LCD; dùng servo (hoặc cơ chế chốt) để mở/đóng.

Kiến trúc hệ thống (block diagram - dạng văn bản)

ESP32 (Wi-Fi, xử lý trung tâm)
├─ MFRC522 (RFID) → xác thực thẻ
├─ Web server / API (Smartphone) → lệnh Open / Close
├─ HC-SR04 (ultrasonic) → phát hiện người đến gần (auto open)
├─ SG90 (servo) → actuator mở/đóng cửa
├─ LCD (I2C 16×2) → hiển thị trạng thái / UID / lỗi
└─ Nguồn (5V cho servo; 3.3V cho ESP32/RFID)
(Tất cả chung mass / GND)

Danh sách linh kiện & vai trò ngắn gọn

ESP32 — bộ xử lý, Wi-Fi server/client, SPI/I2C.

MFRC522 (RFID) — đọc thẻ 13.56 MHz, giao tiếp SPI; cấp 3.3V.

HC-SR04 (ultrasonic) — phát hiện khoảng cách để auto-open; lưu ý điện áp ECHO khi dùng với ESP32.

SG90 (micro servo) — cơ cấu mở cửa mẫu (lá cửa mô hình); nếu lắp cửa thật, cân nhắc solenoid/strike mạnh hơn.

LCD 16×2 (I2C adapter) — hiển thị dễ, chỉ cần SDA/SCL (2 dây) tới ESP32.

Breadboard + jumpers + điện trở (cho voltage divider) — prototyping.

Nguồn 5V (riêng cho servo) — KHÔNG cấp servo từ chân 5V USB của ESP32 nếu tải lớn; servo cần nguồn riêng đủ dòng.

Các điểm kỹ thuật quan trọng (bắt buộc biết trước)

Điện áp & mức logic

MFRC522 và ESP32 hoạt động ở 3.3V → cấp 3.3V cho module RFID.

HC-SR04 truyền thống thường dùng 5V; ECHO có thể trả ~5V — không nối thẳng vào ESP32 (ESP32 chỉ chịu 3.3V). Giải pháp: dùng HC-SR04+ (3.3V-compatible) hoặc mạch chia áp (voltage divider) trên chân ECHO.

Nguồn cho servo

SG90 có dòng hoạt động trung bình ~220–300 mA, dòng stall lớn ~600–800 mA → nếu servo chịu tải (mở cửa thật), cần nguồn 5V riêng và tính dự phòng dòng (khuyến nghị 1A cho từng servo trong trường hợp stall). Chung GND với ESP32 bắt buộc để tín hiệu điều khiển đúng.

Nối chung GND

Luôn chung GND giữa ESP32, nguồn servo, và các module khác.

Bảo vệ chân ESP32

Dùng mạch chia áp hoặc level-shifter cho tín hiệu 5V → 3.3V (HC-SR04 ECHO). Một số HC-SR04 phiên bản mới có sẵn 3.3V-compatible.

Gợi ý chân nối (ví dụ để prototyping trên ESP32)

MFRC522 (SPI): SDA/MOSI/MISO/SCK/IRQ/SS — dùng SPI mặc định của ESP32 (chú ý wiring theo hướng dẫn).

HC-SR04: TRIG → GPIO25, ECHO → (voltage divider) → GPIO26. (chỉ ví dụ, chọn chân không có chức năng đặc biệt).

SG90: PWM control → GPIO27; Vcc servo → 5V external; GND chung.

LCD I2C: SDA → GPIO21, SCL → GPIO22 (I2C mặc định ESP32).

Luồng phần mềm (modules cần viết)

RFID module: đọc UID, tra bảng whitelist, gửi event mở/ghi log. (thời gian debounce để tránh mở liên tục).

Wi-Fi / Web API: ESP32 chạy web server (hoặc REST endpoint) có đường dẫn /open /close; bảo vệ bằng password/token (ít nhất basic auth hoặc token nội bộ). Không để open port không bảo vệ nếu kết nối Internet.

Ultrasonic monitor: đo chu kỳ, nếu < ngưỡng (ví dụ 20–30 cm) trong N lần liên tiếp → trigger auto open (kèm timer tự động đóng sau X giây).

Servo control: smooth sweeping, giới hạn góc an toàn; timeout để trả servo về vị trí đóng nếu không có xác nhận.

LCD / UI: hiển thị status: “Locked”, “Open by RFID (UID)”, “Open by Wi-Fi”, “Sensor Detect”.

Logging: lưu UID + thời gian (lưu local EEPROM/flash hoặc gửi lên server để audit).

An ninh & rủi ro (quan trọng)

Không để API mở ra Internet nếu không mã hóa/đăng nhập an toàn. ESP32 có thể làm HTTPS nhưng cấu hình phức tạp; ở mức đồ án, hạn chế trên mạng LAN và dùng password.

RFID MFRC522 dùng thẻ ISO14443A (MIFARE) — nhiều thẻ rẻ không mã hóa; tránh dùng thẻ có thông tin nhạy cảm, hoặc thêm bước xác thực (PIN/OTP) nếu dùng cho cửa thật.

Cơ chế chốt thực tế: SG90 chỉ phù hợp mô hình/demo. Đối với cửa thật, nên dùng solenoid/strike lock hoặc motor có cơ cấu khóa đạt chuẩn và cơ chế dự phòng (khóa cơ học).

Nguồn & công suất — tính nhanh

Servo SG90: dự trữ ~300 mA hoạt động, lên tới ~700 mA stall → nếu chỉ 1 servo, 2A nguồn 5V là an toàn; nhiều servo thì nhân lên.

ESP32 + RFID + LCD tiêu thụ nhỏ (<200 mA bình thường) — cấp bằng 3.3V onboard hoặc regulator.

Test plan (để report/defense)

Test từng module riêng: RFID đọc đúng UID, LCD hiển thị, HC-SR04 đo khoảng cách chính xác, servo phản hồi PWM.

Test tích hợp: RFID → open (thời gian mở/đóng), Wi-Fi → open/close từ điện thoại (thử nhiều mạng), Sensor auto open.

Test tải & power: đo dòng servo khi mở, test timeout & stall.

Test an ninh: thử gửi lệnh open trái phép (không có token), thử thẻ giả (nếu có thể).

Ghi log và trình bày kết quả: số lần mở, delay, bộ nhớ log.

Các bước phát triển & tài liệu tham khảo để nghiên cứu sâu (mình đã đọc và chọn tài liệu thực tế)

Hướng dẫn nối MFRC522 với ESP32 (wiring + code example).

Lưu ý dùng HC-SR04 với ESP32 (3.3V vs 5V; dùng voltage divider hoặc phiên bản 3.3V-compatible).

Nguồn & dự trữ cho servo (khoảng dòng, stall).

LCD I2C với ESP32 — wiring & library (tiện lợi cho prototyping).

Kiến trúc & best practices cho smart door (mô tả hệ thống, bảo mật, actuator suggestions).

Nếu bạn muốn mình tiếp tục ngay bây giờ, mình có thể làm ngay một trong các nội dung sau (chọn 1, mình sẽ thực hiện luôn trong trả lời tiếp theo):

Sơ đồ breadboard chi tiết (ESP32 + MFRC522 + HC-SR04 + SG90 + I2C LCD + voltage divider, với chú thích chân và dây) — ảnh/bản vẽ để bạn cắm theo.

Bản code mẫu (Arduino IDE) gồm: đọc RFID, web server open/close, HC-SR04 đo khoảng cách, điều khiển servo, và hiển thị trên LCD (kèm comments từng dòng).

Slide 1 phút thuyết trình: mục tiêu, lợi ích, kiến trúc, phần demo, kết luận — chuẩn để bạn thuyết trình trước giảng viên.
