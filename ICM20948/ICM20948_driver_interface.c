#include "ICM20948_driver_interface.h"
#include "Icm20948.h"
#include "twi.h"
#include "app_error.h"
#include "nrf_log.h"
#include "twi.h"
#include "nrfx_gpiote.h"
#include "boards.h"
#include "app_timer.h"
#include "nrf_delay.h"

static const uint8_t dmp3_image[] = {
#include "icm20948_img.dmp3a.h"
};

inv_icm20948_t icm_device;

static const float cfg_mounting_matrix[9]= {
	1.f, 0, 0,
	0, 1.f, 0,
	0, 0, 1.f
};

static void icm20948_apply_mounting_matrix(void){
	int ii;

	for (ii = 0; ii < INV_ICM20948_SENSOR_MAX; ii++) {
		inv_icm20948_set_matrix(&icm_device, cfg_mounting_matrix, ii);
	}
}

/* FSR configurations */
int32_t cfg_acc_fsr = 4; // Default = +/- 4g. Valid ranges: 2, 4, 8, 16
int32_t cfg_gyr_fsr = 2000; // Default = +/- 2000dps. Valid ranges: 250, 500, 1000, 2000

static void icm20948_set_fsr(void){
	inv_icm20948_set_fsr(&icm_device, INV_ICM20948_SENSOR_RAW_ACCELEROMETER, (const void *)&cfg_acc_fsr);
	inv_icm20948_set_fsr(&icm_device, INV_ICM20948_SENSOR_ACCELEROMETER, (const void *)&cfg_acc_fsr);
	inv_icm20948_set_fsr(&icm_device, INV_ICM20948_SENSOR_RAW_GYROSCOPE, (const void *)&cfg_gyr_fsr);
	inv_icm20948_set_fsr(&icm_device, INV_ICM20948_SENSOR_GYROSCOPE, (const void *)&cfg_gyr_fsr);
	inv_icm20948_set_fsr(&icm_device, INV_ICM20948_SENSOR_GYROSCOPE_UNCALIBRATED, (const void *)&cfg_gyr_fsr);
}


void print_sensor_data(void * context, uint8_t sensortype, uint64_t timestamp, const void * data, const void *arg)
{
	(void)context;
	float accel[3];
	int1 = false;
//	NRF_LOG_INFO("print data, sensor type: %d", sensortype);

	switch(sensortype) {
	case INV_ICM20948_SENSOR_ACCELEROMETER:
		memcpy(accel, data, sizeof(accel));
		NRF_LOG_INFO("x:"NRF_LOG_FLOAT_MARKER" y:"NRF_LOG_FLOAT_MARKER, NRF_LOG_FLOAT(accel[0]), NRF_LOG_FLOAT(accel[1]));
		break;
	default:
		return;
	}
}

volatile bool int1 = false;

void int_pin_handler(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
//	NRF_LOG_INFO("INT1");
	int1 = true;
//	inv_icm20948_poll_sensor(&icm_device, (void *)0, print_sensor_data);

//	twim_read_register(NULL, 0x2D, raw, 6);
//	NRF_LOG_INFO("%5d %5d %5d", (int16_t)(raw[0]<<8|raw[1]), (int16_t)(raw[2]<<8|raw[3]), (int16_t)(raw[4]<<8|raw[5]));

}

static void gpiote_init(void)
{
	ret_code_t err_code;

	err_code = nrfx_gpiote_init();
	APP_ERROR_CHECK(err_code);

	nrfx_gpiote_in_config_t in_config = NRFX_GPIOTE_CONFIG_IN_SENSE_HITOLO(true);
	in_config.pull = NRF_GPIO_PIN_PULLUP;

	err_code = nrfx_gpiote_in_init(INT1_PIN, &in_config, int_pin_handler);
	APP_ERROR_CHECK(err_code);

}

static void icm20948_sensor_setup()
{
	uint8_t whoami, result;
	result = inv_icm20948_get_whoami(&icm_device, &whoami);
	NRF_LOG_INFO("whoami: %x", whoami);

	inv_icm20948_soft_reset(&icm_device);
	inv_icm20948_sleep_us(500000);

	//	/* Setup accel and gyro mounting matrix and associated angle for TODO:current?? board */
	inv_icm20948_init_matrix(&icm_device);

	icm20948_apply_mounting_matrix();

	result |= inv_icm20948_initialize(&icm_device, dmp3_image, sizeof(dmp3_image));
	inv_icm20948_register_aux_compass( &icm_device, INV_ICM20948_COMPASS_ID_AK09916, AK0991x_DEFAULT_I2C_ADDR);
	result |= inv_icm20948_initialize_auxiliary(&icm_device);


	icm20948_set_fsr();

	/* re-initialize base state structure */
	result |= inv_icm20948_init_structure(&icm_device);

	if (result)
		NRF_LOG_ERROR("setup sensor");
}

void inv_icm20948_get_st_bias(struct inv_icm20948 * s, int *gyro_bias, int *accel_bias, int * st_bias, int * unscaled){
	int axis, axis_sign;
	int gravity, gravity_scaled;
	int i, t;
	int check;
	int scale;

	/* check bias there ? */
	check = 0;
	for (i = 0; i < 3; i++) {
		if (gyro_bias[i] != 0)
			check = 1;
		if (accel_bias[i] != 0)
			check = 1;
	}

	/* if no bias, return all 0 */
	if (check == 0) {
		for (i = 0; i < 12; i++)
			st_bias[i] = 0;
		return;
	}

	/* dps scaled by 2^16 */
	scale = 65536 / DEF_SELFTEST_GYRO_SENS;

	/* Gyro normal mode */
	t = 0;
	for (i = 0; i < 3; i++) {
		st_bias[i + t] = gyro_bias[i] * scale;
		unscaled[i + t] = gyro_bias[i];
	}
	axis = 0;
	axis_sign = 1;
	if (INV20948_ABS(accel_bias[1]) > INV20948_ABS(accel_bias[0]))
		axis = 1;
	if (INV20948_ABS(accel_bias[2]) > INV20948_ABS(accel_bias[axis]))
		axis = 2;
	if (accel_bias[axis] < 0)
		axis_sign = -1;

	/* gee scaled by 2^16 */
	scale = 65536 / (DEF_ST_SCALE / (DEF_ST_ACCEL_FS_MG / 1000));

	gravity = 32768 / (DEF_ST_ACCEL_FS_MG / 1000) * axis_sign;
	gravity_scaled = gravity * scale;

	/* Accel normal mode */
	t += 3;
	for (i = 0; i < 3; i++) {
		st_bias[i + t] = accel_bias[i] * scale;
		unscaled[i + t] = accel_bias[i];
		if (axis == i) {
			st_bias[i + t] -= gravity_scaled;
			unscaled[i + t] -= gravity;
		}
	}
}

static int unscaled_bias[THREE_AXES * 2];

int icm20948_run_selftest(void){
	static int rc = 0;		// Keep this value as we're only going to do this once.
	int gyro_bias_regular[THREE_AXES];
	int accel_bias_regular[THREE_AXES];
	static int raw_bias[THREE_AXES * 2];

	if (icm_device.selftest_done == 1) {
		NRF_LOG_INFO("Self-test has already run. Skipping.");
	}
	else {
		/*
		* Perform self-test
		* For ICM20948 self-test is performed for both RAW_ACC/RAW_GYR
		*/
		NRF_LOG_INFO("Running self-test...");

		/* Run the self-test */
		rc = inv_icm20948_run_selftest(&icm_device, gyro_bias_regular, accel_bias_regular);
		if ((rc & INV_ICM20948_SELF_TEST_OK) == INV_ICM20948_SELF_TEST_OK) {
			/* On A+G+M self-test success, offset will be kept until reset */
			icm_device.selftest_done = 1;
			icm_device.offset_done = 0;
			rc = 0;
		} else {
			/* On A|G|M self-test failure, return Error */
			NRF_LOG_ERROR("Self-test failure !");
			/* 0 would be considered OK, we want KO */
			rc = INV_ERROR;
		}

		/* It's advised to re-init the icm20948 device after self-test for normal use */
		icm20948_sensor_setup();
		inv_icm20948_get_st_bias(&icm_device, gyro_bias_regular, accel_bias_regular, raw_bias, unscaled_bias);
//		NRF_LOG_INFO("GYR bias (FS=250dps) (dps): x="NRF_LOG_FLOAT_MARKER", y="NRF_LOG_FLOAT_MARKER", z=", NRF_LOG_FLOAT((float)(raw_bias[0] / (float)(1 << 16))), NRF_LOG_FLOAT((float)(raw_bias[1] / (float)(1 << 16))));
//		NRF_LOG_INFO("ACC bias (FS=2g) (g): x="NRF_LOG_FLOAT_MARKER", y="NRF_LOG_FLOAT_MARKER", z=", NRF_LOG_FLOAT((float)(raw_bias[0 + 3] / (float)(1 << 16))), NRF_LOG_FLOAT((float)(raw_bias[1 + 3] / (float)(1 << 16))));
	}

	return rc;
}

void icm20948_init(void)
{
	twi_init();
	gpiote_init();
	/*
	 * Initialize icm20948 serif structure
	 */
	struct inv_icm20948_serif icm20948_serif;
	icm20948_serif.context   = 0; /* no need */
	icm20948_serif.read_reg  = twim_read_register;
	icm20948_serif.write_reg = twim_write_register;
	icm20948_serif.max_read  = 16; /* maximum number of bytes allowed per serial read */
	icm20948_serif.max_write = 16; /* maximum number of bytes allowed per serial write */
	icm20948_serif.is_spi	 = false;

	inv_icm20948_reset_states(&icm_device, &icm20948_serif);

//	inv_icm20948_register_aux_compass(&icm_device, INV_ICM20948_COMPASS_ID_AK09916, AK0991x_DEFAULT_I2C_ADDR);

	icm20948_sensor_setup();

//	inv_icm20948_load(&icm_device, dmp3_image, sizeof(dmp3_image));

//	if(icm20948_run_selftest())
//	{
//		NRF_LOG_ERROR("self test failed");
//		// TODO: invoke error handler
//	}
//	inv_icm20948_set_offset(&icm_device, unscaled_bias);

	uint8_t err;
//	for (uint8_t sensor=0; sensor<INV_ICM20948_SENSOR_MAX; sensor++)
	for (uint8_t sensor=0; sensor<1; sensor++)
	{
		err = inv_icm20948_enable_sensor(&icm_device, sensor, 1);
	}
	err = inv_icm20948_set_sensor_period(&icm_device, 0, 10);

	NRF_LOG_INFO("%d",err);

	nrfx_gpiote_in_event_enable(INT1_PIN, true);
}

uint64_t inv_icm20948_get_time_us(void)
{
	// TODO: change this after implementing global timestamp from the rest of the project???
	uint32_t cnt = app_timer_cnt_get();
//	NRF_LOG_INFO("cnt: %ld", cnt); TODO: make sure there are no overflows for at least 8 hours? in relation to the global timestamp above
	return cnt;
}

//
//void inv_icm20948_sleep(int ms) {
//	delay_ms(ms);
//}

void inv_icm20948_sleep_us(int us){
	nrfx_coredep_delay_us(us);
}



