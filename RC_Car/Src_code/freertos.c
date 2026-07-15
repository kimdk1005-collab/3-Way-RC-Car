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
/* 자율주행 거리 기준과 PWM 튜닝 파라미터 */
#define SENSOR_MAX_CM       400U
#define SENSOR_INVALID      999U

#define FRONT_SAFE_CM       25U     /* 전방 장애물 회피 기준 거리 [cm] */
#define SIDE_EMERGENCY_CM   7U      /* 측면 긴급 회피 기준 거리 [cm] */

#define SPEED_BASE          450     /* 자율주행 기본 PWM */
#define SPEED_SPIN          450     /* 장애물 회피용 제자리 회전 PWM */
#define SPEED_MANUAL        600     /* 스마트폰/컨트롤러 모드 PWM */

#define P_GAIN              15      /* 좌우 거리 오차에 적용하는 비례 제어 이득 */
#define MAX_STEER_OFFSET    200     /* 과도한 조향을 막기 위한 최대 보정량 */
/* USER CODE END PD */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
osMessageQueueId_t btQueueHandle;
osMutexId_t stateMutex;
osMutexId_t sensorMutex;

uint8_t is_auto_mode = 0; /* 0: Smartphone, 1: Autonomous, 2: Controller */
char current_dir = 's';
uint16_t dist_L = SENSOR_MAX_CM;
uint16_t dist_C = SENSOR_MAX_CM;
uint16_t dist_R = SENSOR_MAX_CM;

uint8_t headlight_on = 0; /* 0: OFF, 1: ON */
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
    .priority = (osPriority_t) osPriorityAboveNormal, /* 센서 갱신을 주행 Task보다 우선 */
  };
  SensorTaskHandle = osThreadNew(StartSensorTask, NULL, &SensorTask_attributes);

  const osThreadAttr_t CommTask_attributes = {
    .name = "CommTask",
    .stack_size = 256 * 4,
    .priority = (osPriority_t) osPriorityNormal,
  };
  CommTaskHandle = osThreadNew(StartCommTask, NULL, &CommTask_attributes);
}

/**
  * @brief 차량의 주행 출력과 조명/부저 상태를 갱신하는 Task
  * @note  20 ms 주기로 동작하며, 공유 상태는 Mutex로 복사한 뒤 사용한다.
  */
void StartMotorTask(void *argument)
{
  /* USER CODE BEGIN StartMotorTask */
  for(;;)
  {
    osMutexAcquire(stateMutex, osWaitForever);
    uint8_t mode = is_auto_mode;
    char dir = current_dir;
    osMutexRelease(stateMutex);

    /* 현재 모드를 RGB LED 색상으로 표시한다. */
    if      (mode == 0) LED_SetColor(LED_COLOR_BLUE);
    else if (mode == 1) LED_SetColor(LED_COLOR_GREEN);
    else if (mode == 2) LED_SetColor(LED_COLOR_RED);

    char light_dir = dir;

    if (mode == 0 || mode == 2) {
        /* 스마트폰 수동 모드와 물리 컨트롤러 모드는 동일한 방향 명령을 사용한다. */
        if      (dir == 'f') motor_forward(SPEED_MANUAL);
        else if (dir == 'b') motor_backward(SPEED_MANUAL);
        else if (dir == 'l') motor_spin_left(SPEED_MANUAL);
        else if (dir == 'r') motor_spin_right(SPEED_MANUAL);
        else                 motor_stop();
        light_dir = dir;

    } else if (mode == 1) {
        /* 자율주행 모드: 장애물 회피 우선, 안전 구역에서는 P 제어 수행 */
        osMutexAcquire(sensorMutex, osWaitForever);
        uint16_t l = dist_L;
        uint16_t c = dist_C;
        uint16_t r = dist_R;
        osMutexRelease(sensorMutex);

        /* 0 cm는 유효하지 않은 측정값이므로 회피 판단에서 제외한다. */
        if (c == 0U) c = SENSOR_INVALID;
        if (l == 0U) l = SENSOR_INVALID;
        if (r == 0U) r = SENSOR_INVALID;

        /* 1순위: 전방 장애물 발견 시 더 넓은 방향으로 제자리 회전 */
        if (c < FRONT_SAFE_CM) {
            if (l >= r) {
                motor_spin_left(SPEED_SPIN);
                light_dir = 'l';
            } else {
                motor_spin_right(SPEED_SPIN);
                light_dir = 'r';
            }
        }
        /* 2순위: 측면 충돌이 임박하면 반대 방향으로 즉시 회전 */
        else if (l < SIDE_EMERGENCY_CM) {
            motor_spin_right(SPEED_SPIN);
            light_dir = 'r';
        }
        else if (r < SIDE_EMERGENCY_CM) {
            motor_spin_left(SPEED_SPIN);
            light_dir = 'l';
        }
        /* 3순위: 좌우 거리 오차를 이용해 복도 중앙을 유지 */
        else {
            int16_t diff = (int16_t)l - (int16_t)r;

            /* ±2 cm 이내의 작은 오차는 무시해 조향 헌팅을 줄인다. */
            if (diff >= -2 && diff <= 2) {
                diff = 0;
            }

            int16_t offset = diff * P_GAIN;

            /* 보정량을 제한해 급격한 조향과 오버슈트를 방지한다. */
            if (offset >  MAX_STEER_OFFSET) offset =  MAX_STEER_OFFSET;
            if (offset < -MAX_STEER_OFFSET) offset = -MAX_STEER_OFFSET;

            int16_t left_spd  = (int16_t)SPEED_BASE - offset;
            int16_t right_spd = (int16_t)SPEED_BASE + offset;

            if (left_spd  < 0)    left_spd  = 0;
            if (left_spd  > 1000) left_spd  = 1000;
            if (right_spd < 0)    right_spd = 0;
            if (right_spd > 1000) right_spd = 1000;

            /* 좌우 PWM을 독립 적용해 차등 조향을 구현한다. */
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

/**
  * @brief 좌·중앙·우 초음파 센서를 순차 측정하는 Task
  * @note  센서 간 초음파 간섭을 줄이기 위해 각 측정 사이에 15 ms를 둔다.
  */
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

/**
  * @brief UART ISR이 Queue에 저장한 1-byte 명령을 해석하는 Task
  * @note  방향 명령은 자율주행 모드가 아닐 때만 반영한다.
  */
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
        else if (rx_data == 'T') {
            /* 'L'은 좌회전에 사용하므로 헤드라이트는 'T'로 분리한다. */
            headlight_on = !headlight_on;
        }
        else if (is_auto_mode != 1) {
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

/**
  * @brief 주행 방향에 따라 헤드라이트, 방향지시등, 후진등을 제어한다.
  * @param dir 'f', 'b', 'l', 'r', 's' 중 하나의 주행 상태
  * @note  방향지시등은 HAL_GetTick()을 이용해 500 ms마다 토글한다.
  */
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

/**
  * @brief 초음파 거리값의 작은 노이즈를 완화하고 큰 변화는 즉시 반영한다.
  * @param previous 직전 필터 출력값
  * @param current  새로 측정한 거리값
  * @return 필터링된 거리값
  */
static uint16_t filter_distance(uint16_t previous, uint16_t current)
{
    uint16_t delta;
    uint16_t threshold;

    /* 0 또는 측정 범위를 벗어난 값은 무응답으로 보고 이전 정상값을 유지한다. */
    if (current == 0U || current > SENSOR_MAX_CM) {
        return previous;
    }

    /* 초기 유효값은 필터 지연 없이 바로 사용한다. */
    if (previous == 0U) {
        return current;
    }

    delta = (current > previous) ? (current - previous) : (previous - current);
    threshold = (uint16_t)(previous * 3U / 10U);

    /*
     * 이전값 대비 30%를 초과한 변화는 장애물의 등장/이탈로 판단한다.
     * 큰 변화까지 평균 처리하면 회피 반응이 늦어질 수 있으므로 즉시 반영한다.
     */
    if (delta > threshold) {
        return current;
    }

    /* 작은 변화는 50:50 평균으로 완화한다. */
    return (uint16_t)((previous + current) / 2U);
}
/* USER CODE END Application */
