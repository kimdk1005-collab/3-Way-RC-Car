/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : RC Car Autonomous Driving + LED Mode + Reverse Buzzer
  * [수정본: P-제어 자율주행 복구 + LED/라이트/컨트롤러 모드 유지]
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
/* ─── 자율주행 센서 및 속도 파라미터 ─── */
#define SENSOR_MAX_CM       400U
#define SENSOR_INVALID      999U

#define FRONT_SAFE_CM       25U     // 전방 벽 감지 거리
#define SIDE_EMERGENCY_CM   7U      // 측면 충돌 임박 거리

#define SPEED_BASE          700     // 평소 직진 속도
#define SPEED_SPIN          750     // 회전 속도
#define SPEED_MANUAL        800     // 수동 조종 시 속도

#define P_GAIN              15      // 조향 민감도 (P 제어용) ← 첫 번째 코드 기준 복구
#define MAX_STEER_OFFSET    200     // 최대 조향 꺾임 한계값 ← 복구 (없으면 오버슈트 발생)
/* USER CODE END PD */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
osMessageQueueId_t btQueueHandle;
osMutexId_t stateMutex;
osMutexId_t sensorMutex;

uint8_t is_auto_mode = 0; // 0: 수동(파랑), 1: 자동(녹색), 2: 컨트롤러(빨강)
char current_dir = 's';
uint16_t dist_L = SENSOR_MAX_CM;
uint16_t dist_C = SENSOR_MAX_CM;
uint16_t dist_R = SENSOR_MAX_CM;

uint8_t headlight_on = 0; // 헤드라이트 스위치
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
    .priority = (osPriority_t) osPriorityAboveNormal, // ← 센서 태스크 우선순위 복구
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
 * Motor Task
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

    // 모드에 따른 LED 색상 반영 (수동:파랑, 자동:초록, 컨트롤러:빨강)
    if      (mode == 0) LED_SetColor(LED_COLOR_BLUE);
    else if (mode == 1) LED_SetColor(LED_COLOR_GREEN);
    else if (mode == 2) LED_SetColor(LED_COLOR_RED);

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
        // ── 자율주행 모드 (P-제어 복구) ──
        osMutexAcquire(sensorMutex, osWaitForever);
        uint16_t l = dist_L;
        uint16_t c = dist_C;
        uint16_t r = dist_R;
        osMutexRelease(sensorMutex);

        // [복구] 노이즈(0) 방어: 값 없으면 아주 먼 거리로 간주
        if (c == 0U) c = SENSOR_INVALID;
        if (l == 0U) l = SENSOR_INVALID;
        if (r == 0U) r = SENSOR_INVALID;

        // [복구] 전방 벽 → 스핀 회전
        if (c < FRONT_SAFE_CM) {
            if (l >= r) {
                motor_spin_left(SPEED_SPIN);
                light_dir = 'l';
            } else {
                motor_spin_right(SPEED_SPIN);
                light_dir = 'r';
            }
        }
        // [복구] 측면 긴급 회피
        else if (l < SIDE_EMERGENCY_CM) {
            motor_spin_right(SPEED_SPIN);
            light_dir = 'r';
        }
        else if (r < SIDE_EMERGENCY_CM) {
            motor_spin_left(SPEED_SPIN);
            light_dir = 'l';
        }
        // [복구] P-제어 중앙 유지 직진
        else {
            int16_t diff = (int16_t)l - (int16_t)r;

            // [복구] 데드밴드: ±2cm 이내면 완벽 직진 (흔들림 방지)
            if (diff >= -2 && diff <= 2) {
                diff = 0;
            }

            int16_t offset = diff * P_GAIN;

            // [복구] 조향 한계 클램핑 (오버슈트 방지)
            if (offset >  MAX_STEER_OFFSET) offset =  MAX_STEER_OFFSET;
            if (offset < -MAX_STEER_OFFSET) offset = -MAX_STEER_OFFSET;

            int16_t left_spd  = (int16_t)SPEED_BASE - offset;
            int16_t right_spd = (int16_t)SPEED_BASE + offset;

            if (left_spd  < 0)    left_spd  = 0;
            if (left_spd  > 1000) left_spd  = 1000;
            if (right_spd < 0)    right_spd = 0;
            if (right_spd > 1000) right_spd = 1000;

            // [복구] motor_forward_diff() 사용 (직접 TIM 레지스터 조작 제거)
            motor_forward_diff((uint16_t)left_spd, (uint16_t)right_spd);
            light_dir = 'f';
        }
    }

    Buzzer_Update();
    CarLight_Update(light_dir);

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
            is_auto_mode = 0;
            current_dir = 's';
            motor_stop();
            Buzzer_SetState(BUZZER_IDLE);
        }
        else if (rx_data == 'c' || rx_data == 'C') {
            is_auto_mode = 2;
            motor_stop();
            Buzzer_SetState(BUZZER_IDLE);
        }
        else if (rx_data == 'H' || rx_data == 'h') {
            Buzzer_SetState(BUZZER_HORN);
        }
        else if (rx_data == 'L') {
            headlight_on = !headlight_on;
        }
        else if (is_auto_mode == 0 || is_auto_mode == 2) {
            if (rx_data == 'f' || rx_data == 'F') {
                if (current_dir != 'f') Buzzer_SetState(BUZZER_IDLE);
                current_dir = 'f';
            }
            else if (rx_data == 'b' || rx_data == 'B') {
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

    if (dir == 'b') {
        HAL_GPIO_WritePin(LED_REAR_GPIO_Port, LED_REAR_Pin, GPIO_PIN_SET);
    } else {
        HAL_GPIO_WritePin(LED_REAR_GPIO_Port, LED_REAR_Pin, GPIO_PIN_RESET);
    }

    if (now - last_blink_time >= 500) {
        blink_state = !blink_state;
        last_blink_time = now;
    }

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
 * 노이즈 방지용 이동 평균 필터
 * ============================================================ */
static uint16_t filter_distance(uint16_t previous, uint16_t current)
{
    if (current == 0U || current > SENSOR_MAX_CM) {
        current = SENSOR_MAX_CM;
    }
    if (previous == 0U || previous == SENSOR_MAX_CM) {
        return current;
    }
    // 과거 30%, 현재 70% 비중 (빠른 벽 감지 반응)
    return (uint16_t)((previous * 3U + current * 7U) / 10U);
}
/* USER CODE END Application */
