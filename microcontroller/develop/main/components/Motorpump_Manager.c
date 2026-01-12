/*
 * Motobomba_Manager.c
 *
 *  Created on: 27/12/2025
 *      Author: Lotfi Dalal
 */

#include "Motorpump_Manager.h"
#include "Globals.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"

#include <stdatomic.h>
#include <time.h>

static const char *TAG_MOTORPUMP = "MOTORPUMP";


//static const float vcc_pwm_circuit = 12.0;
static const int pwm_frequency = MOTOBOMBAN_TIMEBASE_PERIOD; 
static const int pwm_clk_frecquency = MOTOBOMBAN_TIMEBASE_RESOLUTION_HZ;
static const int pwm_steps = pwm_clk_frecquency / pwm_frequency;
static const int pwm_one_percent_value = pwm_steps / 100;
static int current_pwm_value = 0;

static motorpumpState_t* motorpump_data = NULL;
static func_callback_t motorpump_event_callback = NULL;
//static func_callback_t turn_off_callback = NULL; 


#define TURN_ON_MOTORPUMP_BIT		BIT1
#define TURN_OFF_MOTORPUMP_BIT		BIT2

static TaskHandle_t xMainMotorpumpTaskHandle = NULL;
static atomic_uint_fast16_t main_motorpump_task_flags = 0; 
static bool timer_mcpwm_enable = true;

static void Motorpump_main_task();
static void Motorpump_turn_on_handle();
static void Motorpump_turn_off_handle();
static void Motorpump_set_pwm_value(uint8_t percent);


static mcpwm_timer_handle_t mcpwm_timer = NULL;
static mcpwm_timer_config_t mcpwm_timer_config = {
    .group_id = 0,
    .clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT,
    .resolution_hz = MOTOBOMBAN_TIMEBASE_RESOLUTION_HZ,
    .period_ticks = MOTOBOMBAN_TIMEBASE_PERIOD,
    .count_mode = MCPWM_TIMER_COUNT_MODE_UP,
};

static mcpwm_oper_handle_t mcpwm_operator = NULL;
static mcpwm_operator_config_t mcpwm_operator_config = {
    .group_id = 0, // operator must be in the same group to the timer
};
static mcpwm_cmpr_handle_t mcpwm_comparator = NULL;
static mcpwm_comparator_config_t mcpwm_comparator_config = {
    .flags.update_cmp_on_tez = true,
};

static mcpwm_gen_handle_t mcpwm_generator = NULL;
static mcpwm_generator_config_t mcpwm_generator_config = {
    .gen_gpio_num = MOTOBOMBAN_GPIO_OUTPUT,
};



void Motorpump_init(motorpumpState_t* _motorpump_data){
	
//    ESP_LOGI(TAG, "Create timer and operator");
    ESP_ERROR_CHECK(mcpwm_new_timer(&mcpwm_timer_config, &mcpwm_timer));

    ESP_ERROR_CHECK(mcpwm_new_operator(&mcpwm_operator_config, &mcpwm_operator));

//    ESP_LOGI(TAG, "Connect timer and operator");
    ESP_ERROR_CHECK(mcpwm_operator_connect_timer(mcpwm_operator, mcpwm_timer));

//    ESP_LOGI(TAG, "Create comparator and generator from the operator");

    ESP_ERROR_CHECK(mcpwm_new_comparator(mcpwm_operator, &mcpwm_comparator_config, &mcpwm_comparator));

    ESP_ERROR_CHECK(mcpwm_new_generator(mcpwm_operator, &mcpwm_generator_config, &mcpwm_generator));

//    ESP_LOGI(TAG, "Set generator action on timer and compare event");
    // go high on counter empty
    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_timer_event(mcpwm_generator,
                                                              MCPWM_GEN_TIMER_EVENT_ACTION(
																  MCPWM_TIMER_DIRECTION_UP, 
																  MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH)));
    // go low on compare threshold
    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_compare_event(mcpwm_generator,
                                                                MCPWM_GEN_COMPARE_EVENT_ACTION(
																	MCPWM_TIMER_DIRECTION_UP, mcpwm_comparator, 
																	MCPWM_GEN_ACTION_LOW)));

    ESP_ERROR_CHECK(mcpwm_timer_enable(mcpwm_timer));
    ESP_ERROR_CHECK(mcpwm_timer_start_stop(mcpwm_timer, MCPWM_TIMER_START_NO_STOP));
    
    motorpump_data = _motorpump_data;
    motorpump_data->state = false;
    
    xTaskCreate(Motorpump_main_task, "MAIN_MOTORPUMP TASK", 3072, NULL, 13, &xMainMotorpumpTaskHandle);
    
    ESP_LOGI(TAG_MOTORPUMP, "Component initialized!");
    
}

void Motorpump_set_callback(func_callback_t _motorpump_event_callback) {
	motorpump_event_callback = _motorpump_event_callback;
}

void Motorpump_main_task() {
	
	uint32_t count = 0;
	uint16_t bits_ask = 0;
	
	
	while (1) {
		
		count = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		
		if (count > 0){
			
			bits_ask = atomic_exchange(&main_motorpump_task_flags, 0);
			
			if (bits_ask & TURN_ON_MOTORPUMP_BIT) {
				Motorpump_turn_on_handle();
			}
			
			if (bits_ask & TURN_OFF_MOTORPUMP_BIT) {
				Motorpump_turn_off_handle();
			}
			
		}
	}
}


void Motorpump_turn_on() {
	atomic_fetch_or(&main_motorpump_task_flags, TURN_ON_MOTORPUMP_BIT);
	xTaskNotify(xMainMotorpumpTaskHandle, 1, eIncrement);
}


void Motorpump_turn_off() {
	atomic_fetch_or(&main_motorpump_task_flags, TURN_OFF_MOTORPUMP_BIT);
	xTaskNotify(xMainMotorpumpTaskHandle, 1, eIncrement);
}


void Motorpump_turn_on_handle() {
	
	if (motorpump_data->state) {
		ESP_LOGI(TAG_MOTORPUMP, "Motorpump already on!");
		return;
	}
	
	if (!timer_mcpwm_enable) {
		mcpwm_timer_enable(mcpwm_timer);
	}
	
	time_t tiemstamp;
	time(&tiemstamp);
	motorpump_data->timestamp = (long long)tiemstamp;
	motorpump_data->state = true;
	
	motorpump_event_callback();
	
	while (current_pwm_value < 100) {
		current_pwm_value ++;
		Motorpump_set_pwm_value(current_pwm_value);
		vTaskDelay(pdMS_TO_TICKS(10));
	}
}


void Motorpump_turn_off_handle() {
	if (!motorpump_data->state) {
		ESP_LOGI(TAG_MOTORPUMP, "Motorpump already off!");
		return;
	}
	
	while (current_pwm_value > 0) {
		current_pwm_value --;
		Motorpump_set_pwm_value(current_pwm_value);
		vTaskDelay(pdMS_TO_TICKS(10));
	}
		
	if (timer_mcpwm_enable) {
		mcpwm_timer_disable(mcpwm_timer);
	}
	
	time_t tiemstamp;
	time(&tiemstamp);
	motorpump_data->timestamp = (long long)tiemstamp;
	motorpump_data->state = false;

	motorpump_event_callback();
}


void Motorpump_set_pwm_value(uint8_t percent) {
	int comparator_value = percent * pwm_one_percent_value;
	
	mcpwm_comparator_set_compare_value(mcpwm_comparator, comparator_value);
	
}






