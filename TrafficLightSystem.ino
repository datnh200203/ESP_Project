#include <Arduino_FreeRTOS.h>
#include <Arduino.h>
#include <task.h>

// Định nghĩa chân GPIO cho đèn LED
#define LED1_RED_PIN    16
#define LED1_YELLOW_PIN 17
#define LED1_GREEN_PIN  5
#define LED2_RED_PIN    4
#define LED2_YELLOW_PIN 0
#define LED2_GREEN_PIN  2

// Chân GPIO cho nút nhấn
#define BUTTON_PIN 14  // Chân GPIO cho nút nhấn COLOR
#define MODE_PIN 13     // Chân GPIO cho nút nhấn MODE

// Định nghĩa chế độ
#define AUTO    0
#define MANUAL  1

// Biến toàn cục để lưu trữ chế độ hiện tại
int currentMode = AUTO; // Khởi tạo ở chế độ AUTO

// Khai báo nguyên mẫu hàm vTimerCallback 
void vTimerCallback(TimerHandle_t xTimer);

// Hàm callback được gọi khi timer hết hạn
void vTimerCallback(TimerHandle_t xTimer) {
  // Lấy ID của timer
  int timerID = (int)pvTimerGetTimerID(xTimer);

  // Biến static để theo dõi trạng thái đèn
  static int autoCase = 1;

  // Xử lý dựa trên ID của timer
  switch (timerID) {
    case 0: // Timer cho pha 1 (đèn 1 xanh, đèn 2 đỏ) - 30 giây
      autoCase = 2; // Chuyển sang pha 2
      xTimerStart(xTimerYellow1, 0); // Bắt đầu timer cho pha 2
      break;
    case 1: // Timer cho pha 2 (đèn 1 vàng, đèn 2 đỏ) - 5 giây
      autoCase = 3; // Chuyển sang pha 3
      xTimerStart(xTimerGreen2, 0); // Bắt đầu timer cho pha 3
      break;
    case 2: // Timer cho pha 3 (đèn 1 đỏ, đèn 2 xanh) - 30 giây
      autoCase = 4; // Chuyển sang pha 4
      xTimerStart(xTimerYellow2, 0); // Bắt đầu timer cho pha 4
      break;
    case 3: // Timer cho pha 4 (đèn 1 đỏ, đèn 2 vàng) - 5 giây
      autoCase = 1; // Chuyển sang pha 1
      xTimerStart(xTimerGreen1, 0); // Bắt đầu timer cho pha 1
      break;
  }

  // Cập nhật trạng thái đèn dựa trên autoCase
  switch (autoCase) {
    case 1:
      digitalWrite(LED1_GREEN_PIN, HIGH);
      digitalWrite(LED1_YELLOW_PIN, LOW);
      digitalWrite(LED1_RED_PIN, LOW);
      digitalWrite(LED2_GREEN_PIN, LOW);
      digitalWrite(LED2_YELLOW_PIN, LOW);
      digitalWrite(LED2_RED_PIN, HIGH);
      break;
    case 2:
      digitalWrite(LED1_GREEN_PIN, LOW);
      digitalWrite(LED1_YELLOW_PIN, HIGH);
      digitalWrite(LED1_RED_PIN, LOW);
      digitalWrite(LED2_GREEN_PIN, LOW);
      digitalWrite(LED2_YELLOW_PIN, LOW);
      digitalWrite(LED2_RED_PIN, HIGH);
      break;
    case 3:
      digitalWrite(LED1_GREEN_PIN, LOW);
      digitalWrite(LED1_YELLOW_PIN, LOW);
      digitalWrite(LED1_RED_PIN, HIGH);
      digitalWrite(LED2_GREEN_PIN, HIGH);
      digitalWrite(LED2_YELLOW_PIN, LOW);
      digitalWrite(LED2_RED_PIN, LOW);
      break;
    case 4:
      digitalWrite(LED1_GREEN_PIN, LOW);
      digitalWrite(LED1_YELLOW_PIN, LOW);
      digitalWrite(LED1_RED_PIN, HIGH);
      digitalWrite(LED2_GREEN_PIN, LOW);
      digitalWrite(LED2_YELLOW_PIN, HIGH);
      digitalWrite(LED2_RED_PIN, LOW);
      break;
  }
}

// Task quản lý chế độ (vModeManagerTask)
void vModeManagerTask(void *pvParameters) {
  // Tạo task vManualModeTask và lưu TaskHandle_t
  TaskHandle_t xManualTaskHandle;
  xTaskCreate(vManualModeTask, "ManualModeTask", 1024, NULL, 1, &xManualTaskHandle);

  // Tạo task vAutoModeTask và lưu TaskHandle_t
  TaskHandle_t xAutoTaskHandle;
  xTaskCreate(vAutoModeTask, "AutoModeTask", 1024, NULL, 1, &xAutoTaskHandle);
  vTaskSuspend(xAutoTaskHandle); // Suspend vAutoModeTask ban đầu

  for (;;) {
    // Đọc giá trị MODE từ nút nhấn
    int MODE = digitalRead(MODE_PIN);

    if (MODE) { // Nếu MODE thay đổi
      if (currentMode == AUTO) {
        currentMode = MANUAL;
        // Tắt tất cả đèn LED
        digitalWrite(LED1_RED_PIN, LOW);
        digitalWrite(LED1_YELLOW_PIN, LOW);
        digitalWrite(LED1_GREEN_PIN, LOW);
        digitalWrite(LED2_RED_PIN, LOW);
        digitalWrite(LED2_YELLOW_PIN, LOW);
        digitalWrite(LED2_GREEN_PIN, LOW);

        // Resume manual task sử dụng TaskHandle_t
        vTaskResume(xManualTaskHandle);

        // Suspend auto task sử dụng TaskHandle_t
        vTaskSuspend(xAutoTaskHandle);
      } else {
        currentMode = AUTO;

        // Suspend manual task sử dụng TaskHandle_t
        vTaskSuspend(xManualTaskHandle);

        // Resume auto task sử dụng TaskHandle_t
        vTaskResume(xAutoTaskHandle);
      }
    }

        vTaskDelay(pdMS_TO_TICKS(100)); // Kiểm tra MODE mỗi 100ms
  }
}

// Task cho chế độ thủ công (vManualModeTask)
void vManualModeTask(void *pvParameters) {
  int manualCase = 1; // Khởi tạo trạng thái ban đầu
  int COLOR; // Biến để lưu giá trị nút nhấn

  for (;;) {
    switch (manualCase) {
      case 1:
        digitalWrite(LED1_GREEN_PIN, HIGH);
        digitalWrite(LED2_RED_PIN, HIGH);
        // Đọc giá trị COLOR từ nút nhấn
        COLOR = !digitalRead(BUTTON_PIN); // Đảo ngược giá trị đọc được
        if (COLOR) {
          manualCase = 2;
        }
        break;
      case 2:
        digitalWrite(LED1_YELLOW_PIN, HIGH);
        digitalWrite(LED2_RED_PIN, HIGH);
        vTaskDelay(pdMS_TO_TICKS(3000)); // Delay 3s
        manualCase = 3;
        break;
      case 3:
        digitalWrite(LED1_RED_PIN, HIGH);
        digitalWrite(LED2_GREEN_PIN, HIGH);
        // Đọc giá trị COLOR từ nút nhấn
        COLOR = !digitalRead(BUTTON_PIN); // Đảo ngược giá trị đọc được
        if (COLOR) {
          manualCase = 4;
        }
        break;
      case 4: // Chuyển đèn 2 sang vàng, rồi đỏ, sau đó đèn 1 sang xanh
        digitalWrite(LED2_YELLOW_PIN, HIGH);
        vTaskDelay(pdMS_TO_TICKS(3000)); // Delay 3s
        digitalWrite(LED2_YELLOW_PIN, LOW);
        digitalWrite(LED2_RED_PIN, HIGH);
        digitalWrite(LED1_RED_PIN, LOW);
        digitalWrite(LED1_GREEN_PIN, HIGH);
        manualCase = 1;
        break;
    }
    vTaskDelay(pdMS_TO_TICKS(100)); // Delay 100ms để nhường CPU
  }
}

// Tạo các timer
TimerHandle_t xTimerGreen1 = xTimerCreate("TimerGreen1", pdMS_TO_TICKS(30000), pdFALSE, (void *)0, vTimerCallback);
TimerHandle_t xTimerYellow1 = xTimerCreate("TimerYellow1", pdMS_TO_TICKS(5000), pdFALSE, (void *)1, vTimerCallback);
TimerHandle_t xTimerGreen2 = xTimerCreate("TimerGreen2", pdMS_TO_TICKS(30000), pdFALSE, (void *)2, vTimerCallback);
TimerHandle_t xTimerYellow2 = xTimerCreate("TimerYellow2", pdMS_TO_TICKS(5000), pdFALSE, (void *)3, vTimerCallback);

// Task cho chế độ tự động (vAutoModeTask)
void vAutoModeTask(void *pvParameters) {
  // Bắt đầu timer cho pha 1
  xTimerStart(xTimerGreen1, 0);

  for (;;) {
   
    vTaskDelay(pdMS_TO_TICKS(1000)); // Delay 1 giây để nhường CPU
  }
}

void setup() {
  Serial.begin(115200);

  // Khởi tạo các chân GPIO cho đèn LED
  pinMode(LED1_RED_PIN, OUTPUT);
  pinMode(LED1_YELLOW_PIN, OUTPUT);
  pinMode(LED1_GREEN_PIN, OUTPUT);
  pinMode(LED2_RED_PIN, OUTPUT);
  pinMode(LED2_YELLOW_PIN, OUTPUT);
  pinMode(LED2_GREEN_PIN, OUTPUT);

  // Khởi tạo chân nút nhấn với điện trở kéo lên
  pinMode(BUTTON_PIN, INPUT_PULLUP);  // Cho nút nhấn COLOR
  pinMode(MODE_PIN, INPUT_PULLUP);     // Cho nút nhấn MODE

  // Tạo task vModeManagerTask
  xTaskCreate(vModeManagerTask, "ModeManagerTask", 1024, NULL, 1, NULL);
}

void loop() {
  
}
