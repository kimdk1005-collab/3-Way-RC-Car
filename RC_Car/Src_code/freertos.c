/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : RC Car Autonomous Driving (Continuous P-Control)
  * [최종 완료본: 도근님 원본 로직 100% 이식 + 3색 LED/라이트/부저 완벽 통합]
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdlib.h>
#include "motor.h"
#include "ultrasonic.h"
#include "bluetooth.h"
#include "buzzer.h"
#include "led.h"
#include "tim.h"
/* USER CODE END Includes */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* ─── 센서 및 튜닝 파라미터 (Continuous P-Control 전용) ─── */
#define SENSOR_MAX_CM       400U
#define SENSOR_INVALID      999U

#define FRONT_SAFE_CM       25U     // 전방 벽 감지 거리 (이보다 가까우면 휙 꺾음!)
#define SIDE_EMERGENCY_CM   7U      // 측면 충돌 임박 (즉각 회피)

#define SPEED_BASE          450     // 평소 직진 속도
#define SPEED_SPIN          450     // 코너에서 휙 꺾을 때의 회전 속도
#define SPEED_MANUAL        600     // 수동 조종 시 속도

#define P_GAIN              15      // 조향 민감도 (클수록 중앙으로 강하게 들어옴)
#define MAX_STEER_OFFSET    200     // 최대 조향 꺾임 한계값
/* USER CODE END PD */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
osMessageQueueId_t btQueueHandle;
osMutexId_t stateMutex;
osMutexId_t sensorMutex;

uint8_t is_auto_mode = 0; // 0: 수동(파랑), 1: 자동(초록), 2: 컨트롤러(빨강)
char current_dir = 's';
uint16_t dist_L = SENSOR_MAX_CM;
uint16_t dist_C = SENSOR_MAX_CM;
uint16_t dist_R = SENSOR_MAX_CM;

uint8_t headlight_on = 0; // 헤드라이트 껐다/켜기 스위치
/* USER CODE END Variables */

osThreadId_t MotorTaskHandle;
osThreadId_t SensorTaskHandle;
osThreadId_t CommTaskHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
static uint16_t filter_distance(uint16_t previous, uint16_t current);
void CarLight_Update(char dir);
/* USER CODE END FunctionPrototypes */

void StartMotorTask(void *argument);
void StartSensorTask(void *argument);
void StartCommTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN RTOS_MUTEX */
  const osMutexAttr_t stateMutex_attributes = { .name = "stateMutex" };
  stateMutex = osMutexNew(&stateMutex_attributes);

  const osMutexAttr_t sensorMutex_attributes = { .name = "sensorMutex" };
  sensorMutex = osMutexNew(&sensorMutex_attributes);
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_QUEUES */
  const osMessageQueueAttr_t btQueue_attributes = { .name = "btQueue" };
  btQueueHandle = osMessageQueueNew(16, sizeof(uint8_t), &btQueue_attributes);
  /* USER CODE END RTOS_QUEUES */

  const osThreadAttr_t MotorTask_attributes = {
    .name = "MotorTask",
    .stack_size = 256 * 4,
    .priority = (osPriority_t) osPriorityNormal,
  };
  MotorTaskHandle = osThreadNew(StartMotorTask, NULL, &MotorTask_attributes);

  const osThreadAttr_t SensorTask_attributes = {
    .name = "SensorTask",
    .stack_size = 256 * 4,
    .priority = (osPriority_t) osPriorityAboveNormal,
  };
  SensorTaskHandle = osThreadNew(StartSensorTask, NULL, &SensorTask_attributes);

  const osThreadAttr_t CommTask_attributes = {
    .name = "CommTask",
    .stack_size = 256 * 4,
    .priority = (osPriority_t) osPriorityNormal,
  };
  CommTaskHandle = osThreadNew(StartCommTask, NULL, &CommTask_attributes);
}

/* ============================================================
 * Motor Task (차량 주행 및 LED/Buzzer 상태 갱신)
 * ============================================================ */
void StartMotorTask(void *argument)
{
  /* USER CODE BEGIN StartMotorTask */
  for(;;)
  {
    osMutexAcquire(stateMutex, osWaitForever);
    uint8_t mode = is_auto_mode;
    char dir = current_dir;
    osMutexRelease(stateMutex);

    // 상태 표시 LED 색상 반영
    if      (mode == 0) LED_SetColor(LED_COLOR_BLUE);   // 스마트폰 수동
    else if (mode == 1) LED_SetColor(LED_COLOR_GREEN);  // 자율주행
    else if (mode == 2) LED_SetColor(LED_COLOR_RED);    // 조이스틱 컨트롤러

    char light_dir = dir;

    if (mode == 0 || mode == 2) {
        // ── 수동 / 컨트롤러 제어 모드 ──
        if      (dir == 'f') motor_forward(SPEED_MANUAL);
        else if (dir == 'b') motor_backward(SPEED_MANUAL);
        else if (dir == 'l') motor_spin_left(SPEED_MANUAL);
        else if (dir == 'r') motor_spin_right(SPEED_MANUAL);
        else                 motor_stop();
        light_dir = dir;

    } else if (mode == 1) {
        // ── 자율주행 모드 (도근님의 Continuous P-Control) ──
        osMutexAcquire(sensorMutex, osWaitForever);
        uint16_t l = dist_L;
        uint16_t c = dist_C;
        uint16_t r = dist_R;
        osMutexRelease(sensorMutex);

        if (c == 0U) c = SENSOR_INVALID;
        if (l == 0U) l = SENSOR_INVALID;
        if (r == 0U) r = SENSOR_INVALID;

        // [1] 전방 회피 (Spin Turn)
        if (c < FRONT_SAFE_CM) {
            if (l >= r) {
                motor_spin_left(SPEED_SPIN);
                light_dir = 'l';
            } else {
                motor_spin_right(SPEED_SPIN);
                light_dir = 'r';
            }
        }
        // [2] 측면 긴급 회피
        else if (l < SIDE_EMERGENCY_CM) {
            motor_spin_right(SPEED_SPIN);
            light_dir = 'r';
        }
        else if (r < SIDE_EMERGENCY_CM) {
            motor_spin_left(SPEED_SPIN);
            light_dir = 'l';
        }
        // [3] 안전 구역 - 비례 제어 (P-Control) 중앙선 유지
        else {
            int16_t diff = (int16_t)l - (int16_t)r;

            // 흔들림 방지 데드밴드
            if (diff >= -2 && diff <= 2) {
                diff = 0;
            }

            int16_t offset = diff * P_GAIN;

            if (offset >  MAX_STEER_OFFSET) offset =  MAX_STEER_OFFSET;
            if (offset < -MAX_STEER_OFFSET) offset = -MAX_STEER_OFFSET;

            int16_t calc_left  = (int16_t)SPEED_BASE - offset;
            int16_t calc_right = (int16_t)SPEED_BASE + offset;

            // 역회전 방지
            if (calc_left < 0) calc_left = 0;
            if (calc_right < 0) calc_right = 0;

            motor_forward_diff((uint16_t)calc_left, (uint16_t)calc_right);
            light_dir = 'f';
        }
    }

    // 논블로킹 하드웨어 업데이트 (딜레이 없이 즉각 반응)
    Buzzer_Update();
    CarLight_Update(light_dir);

    // 즉각적인 반응을 위해 20ms 주기 사용 (딜레이 최소화)
    osDelay(20);
  }
  /* USER CODE END StartMotorTask */
}

/* ============================================================
 * Sensor Task
 * ============================================================ */
void StartSensorTask(void *argument)
{
  /* USER CODE BEGIN StartSensorTask */
  for(;;)
  {
    uint16_t raw_l = get_distance_left();
    osDelay(15);
    uint16_t raw_c = get_distance_center();
    osDelay(15);
    uint16_t raw_r = get_distance_right();
    osDelay(15);

    osMutexAcquire(sensorMutex, osWaitForever);
    dist_L = filter_distance(dist_L, raw_l);
    dist_C = filter_distance(dist_C, raw_c);
    dist_R = filter_distance(dist_R, raw_r);
    osMutexRelease(sensorMutex);
  }
  /* USER CODE END StartSensorTask */
}

/* ============================================================
 * Comm Task
 * ============================================================ */
void StartCommTask(void *argument)
{
  /* USER CODE BEGIN StartCommTask */
  uint8_t rx_data;
  for(;;)
  {
    if (osMessageQueueGet(btQueueHandle, &rx_data, NULL, osWaitForever) == osOK) {
        osMutexAcquire(stateMutex, osWaitForever);

        if (rx_data == 'a' || rx_data == 'A') {
            is_auto_mode = 1;
            motor_stop();
            Buzzer_SetState(BUZZER_IDLE);
        }
        else if (rx_data == 'm' || rx_data == 'M') {
            is_auto_mode = 0; // 수동 모드
            current_dir = 's';
            motor_stop();
            Buzzer_SetState(BUZZER_IDLE);
        }
        else if (rx_data == 'c' || rx_data == 'C') {
            is_auto_mode = 2; // 컨트롤러 모드
            motor_stop();
            Buzzer_SetState(BUZZER_IDLE);
        }
        else if (rx_data == 'H' || rx_data == 'h') {
            // 경적 (Horn)
            Buzzer_SetState(BUZZER_HORN);
        }
        else if (rx_data == 'T') {
            // 헤드라이트 토글
            headlight_on = !headlight_on;
        }
        else if (is_auto_mode != 1) {
            // 수동(0) 또는 컨트롤러(2) 일 때만 방향키 허용
            if (rx_data == 'f' || rx_data == 'F') {
                if (current_dir != 'f') Buzzer_SetState(BUZZER_IDLE);
                current_dir = 'f';
            }
            else if (rx_data == 'b' || rx_data == 'B') {
                // 후진 기어 시 엘리제를 위하여 멜로디 또는 경고음
                if (current_dir != 'b') Buzzer_SetState(BUZZER_REVERSE_BEEP);
                current_dir = 'b';
            }
            else if (rx_data == 'l' || rx_data == 'L') {
                if (current_dir != 'l') Buzzer_SetState(BUZZER_IDLE);
                current_dir = 'l';
            }
            else if (rx_data == 'r' || rx_data == 'R') {
                if (current_dir != 'r') Buzzer_SetState(BUZZER_IDLE);
                current_dir = 'r';
            }
            else if (rx_data == 's' || rx_data == 'S') {
                if (current_dir != 's') Buzzer_SetState(BUZZER_IDLE);
                current_dir = 's';
            }
        }

        osMutexRelease(stateMutex);
    }
  }
  /* USER CODE END StartCommTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* ============================================================
 * 실제 차량 조명 제어 (헤드라이트, 깜빡이, 후진등)
 * ============================================================ */
void CarLight_Update(char dir) {
    static uint32_t last_blink_time = 0;
    static uint8_t blink_state = 0;
    uint32_t now = HAL_GetTick();

    // 후진등 제어
    if (dir == 'b') {
        HAL_GPIO_WritePin(LED_REAR_GPIO_Port, LED_REAR_Pin, GPIO_PIN_SET);
    } else {
        HAL_GPIO_WritePin(LED_REAR_GPIO_Port, LED_REAR_Pin, GPIO_PIN_RESET);
    }

    // 깜빡이 타이머 (500ms 주기)
    if (now - last_blink_time >= 500) {
        blink_state = !blink_state;
        last_blink_time = now;
    }

    // 방향에 따른 앞면 LED 제어 (방향지시등 우선, 아니면 헤드라이트 상태 반영)
    if (dir == 'l') {
        HAL_GPIO_WritePin(LED_FRONT_L_GPIO_Port, LED_FRONT_L_Pin, blink_state ? GPIO_PIN_SET : GPIO_PIN_RESET);
        HAL_GPIO_WritePin(LED_FRONT_R_GPIO_Port, LED_FRONT_R_Pin, headlight_on ? GPIO_PIN_SET : GPIO_PIN_RESET);
    }
    else if (dir == 'r') {
        HAL_GPIO_WritePin(LED_FRONT_L_GPIO_Port, LED_FRONT_L_Pin, headlight_on ? GPIO_PIN_SET : GPIO_PIN_RESET);
        HAL_GPIO_WritePin(LED_FRONT_R_GPIO_Port, LED_FRONT_R_Pin, blink_state ? GPIO_PIN_SET : GPIO_PIN_RESET);
    }
    else {
        HAL_GPIO_WritePin(LED_FRONT_L_GPIO_Port, LED_FRONT_L_Pin, headlight_on ? GPIO_PIN_SET : GPIO_PIN_RESET);
        HAL_GPIO_WritePin(LED_FRONT_R_GPIO_Port, LED_FRONT_R_Pin, headlight_on ? GPIO_PIN_SET : GPIO_PIN_RESET);
    }
}

/* ============================================================
 * 노이즈 방지용 센서 필터 (30% 급변 감지)
 * ============================================================ */
static uint16_t filter_distance(uint16_t previous, uint16_t current)
{
    if (current == 0U || current > SENSOR_MAX_CM) {
        return previous;
    }
    if (previous == 0U) {
        return current;
    }

    uint16_t delta = (current > previous) ? (current - previous) : (previous - current);
    uint16_t threshold = previous * 3U / 10U;

    // 30% 이상 거리가 급변하면 즉시 100% 반영 (벽이 나타나거나 사라짐)
    if (delta > threshold) {
        return current;
    }

    // 그 외 평상시에는 50:50 필터링으로 부드러운 측정 유지
    return (previous + current) / 2U;
}
/* USER CODE END Application */
