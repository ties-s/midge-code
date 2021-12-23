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
#include "app_scheduler.h"
#include "storage.h"
#include "systick_lib.h"

static const uint8_t dmp3_image[] = {
#include "icm20948_img.dmp3a.h"
};

inv_icm20948_t icm_device;

imu_sample_t imu_buffer[MAX_IMU_SOURCES][2][IMU_BUFFER_SIZE];

const char *imu_sensor_name[MAX_IMU_SOURCES] = {"accel", "gyr", "mag", "rotation"};

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

ret_code_t icm20948_set_fsr(uint32_t acc_fsr, uint32_t gyr_fsr)
{
	ret_code_t err = NRF_SUCCESS;

	err = inv_icm20948_set_fsr(&icm_device, INV_ICM20948_SENSOR_ACCELEROMETER, (const void *)&acc_fsr);
	err |= inv_icm20948_set_fsr(&icm_device, INV_ICM20948_SENSOR_GYROSCOPE, (const void *)&gyr_fsr);

	return err;
}

ret_code_t icm20948_disable_sensors(void)
{
	ret_code_t err = NRF_SUCCESS;

	for (uint8_t sensor=0; sensor<INV_ICM20948_SENSOR_MAX; sensor++)
		err |= inv_icm20948_enable_sensor(&icm_device, sensor, 0);

	nrfx_gpiote_in_event_disable(INT1_PIN);

	return err;
}

ret_code_t icm20948_enable_sensors(void)
{
	ret_code_t err = NRF_SUCCESS;
	err |= inv_icm20948_enable_sensor(&icm_device, INV_ICM20948_SENSOR_ACCELEROMETER, 1);
	err |= inv_icm20948_enable_sensor(&icm_device, INV_ICM20948_SENSOR_GYROSCOPE, 1);
	err |= inv_icm20948_enable_sensor(&icm_device, INV_ICM20948_SENSOR_GEOMAGNETIC_FIELD, 1);
	err |= inv_icm20948_enable_sensor(&icm_device, INV_ICM20948_SENSOR_ROTATION_VECTOR, 1);

	nrfx_gpiote_in_event_enable(INT1_PIN, true);

	return err;
}

ret_code_t icm20948_set_datarate(uint8_t datarate)
{
	ret_code_t err = NRF_SUCCESS;

	for (uint8_t sensor=0; sensor<INV_ICM20948_SENSOR_MAX; sensor++)
		err |= inv_icm20948_set_sensor_period(&icm_device, sensor, 1000/datarate); // 1125Hz is the internal clock. gyro ODR is always a fraction of this

	return err;
}


void print_sensor_data(void * context, uint8_t sensortype, uint64_t timestamp, const void * data, const void *arg)
{
	ret_code_t err_code;
	(void)context;
	data_source_info_t data_source_info;
	data_source_info.data_source = IMU;
	static uint8_t active_buffer[MAX_IMU_SOURCES];
	static uint16_t count[MAX_IMU_SOURCES];
	imu_sample_t sample;

	switch(sensortype) {

	case INV_ICM20948_SENSOR_ACCELEROMETER:

		sample.timestamp = timestamp;
		memcpy(sample.axis, data, 12);
		memcpy( &imu_buffer[ACCEL][active_buffer[ACCEL]][count[ACCEL]++], &sample, sizeof(sample));

		if (count[ACCEL] < IMU_BUFFER_SIZE)
			break;

		// change buffers, send to sd for writting
		data_source_info.imu_source_info.imu_source = ACCEL;
		data_source_info.imu_source_info.imu_buffer = &imu_buffer[ACCEL][active_buffer[ACCEL]];
		err_code = app_sched_event_put(&data_source_info, sizeof(data_source_info), sd_write);
		APP_ERROR_CHECK(err_code);
		active_buffer[ACCEL] = !active_buffer[ACCEL];
		count[ACCEL] = 0;
		break;

	case INV_ICM20948_SENSOR_GYROSCOPE:
		sample.timestamp = timestamp;
		memcpy(sample.axis, data, 12);
		memcpy( &imu_buffer[GYRO][active_buffer[GYRO]][count[GYRO]++], &sample, sizeof(sample));

		if (count[GYRO] < IMU_BUFFER_SIZE)
			break;

		// change buffers, send to sd for writting
		data_source_info.imu_source_info.imu_source = GYRO;
		data_source_info.imu_source_info.imu_buffer = &imu_buffer[GYRO][active_buffer[GYRO]];
		err_code = app_sched_event_put(&data_source_info, sizeof(data_source_info), sd_write);
		APP_ERROR_CHECK(err_code);
		active_buffer[GYRO] = !active_buffer[GYRO];
		count[GYRO] = 0;
		break;

	case INV_ICM20948_SENSOR_GEOMAGNETIC_FIELD:
		sample.timestamp = timestamp;
		memcpy(sample.axis, data, 12);
		memcpy( &imu_buffer[MAG][active_buffer[MAG]][count[MAG]++], &sample, sizeof(sample));

		if (count[MAG] < IMU_BUFFER_SIZE)
			break;

		// change buffers, send to sd for writting
		data_source_info.imu_source_info.imu_source = MAG;
		data_source_info.imu_source_info.imu_buffer = &imu_buffer[MAG][active_buffer[MAG]];
		err_code = app_sched_event_put(&data_source_info, sizeof(data_source_info), sd_write);
		APP_ERROR_CHECK(err_code);
		active_buffer[MAG] = !active_buffer[MAG];
		count[MAG] = 0;
		break;

	case INV_ICM20948_SENSOR_ROTATION_VECTOR:
		sample.timestamp = timestamp;
		memcpy(sample.quat, data, 16);
		memcpy( &imu_buffer[ROTATION_VECTOR][active_buffer[ROTATION_VECTOR]][count[ROTATION_VECTOR]++], &sample, sizeof(sample));

		if (count[ROTATION_VECTOR] < IMU_BUFFER_SIZE)
			break;

		// change buffers, send to sd for writting
		data_source_info.imu_source_info.imu_source = ROTATION_VECTOR;
		data_source_info.imu_source_info.imu_buffer = &imu_buffer[ROTATION_VECTOR][active_buffer[ROTATION_VECTOR]];
		err_code = app_sched_event_put(&data_source_info, sizeof(data_source_info), sd_write);
		APP_ERROR_CHECK(err_code);
		active_buffer[ROTATION_VECTOR] = !active_buffer[ROTATION_VECTOR];
		count[ROTATION_VECTOR] = 0;
		break;

	default:
		return;
	}
}

void icm20948_service_isr(void * p_event_data, uint16_t event_size)
{
	inv_icm20948_poll_sensor(&icm_device, (void *)0, print_sensor_data);
}

static uint32_t icm20948_sensor_setup()
{
	uint8_t whoami, result;
	result = inv_icm20948_get_whoami(&icm_device, &whoami);
	NRF_LOG_INFO("whoami: %x", whoami);

	inv_icm20948_soft_reset(&icm_device);
	inv_icm20948_sleep_us(500000);

	// Setup accel and gyro mounting matrix and associated angle
	inv_icm20948_init_matrix(&icm_device);

	icm20948_apply_mounting_matrix();

	result |= inv_icm20948_initialize(&icm_device, dmp3_image, sizeof(dmp3_image));
	inv_icm20948_register_aux_compass( &icm_device, INV_ICM20948_COMPASS_ID_AK09916, AK0991x_DEFAULT_I2C_ADDR);
	result |= inv_icm20948_initialize_auxiliary(&icm_device);

//	icm20948_set_fsr(); will be set when we start the sensor

	/* re-initialize base state structure */
	result |= inv_icm20948_init_structure(&icm_device);

	return result;
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

ret_code_t icm20948_init(void)
{
	uint32_t err;

	err = twi_init();
	if (err) return err;

	/*
	 * Initialize icm20948 serif structure
	 */
	struct inv_icm20948_serif icm20948_serif;
	icm20948_serif.context   = 0; /* no need */
	icm20948_serif.read_reg  = twim_read_register;
	icm20948_serif.write_reg = twim_write_register;
	icm20948_serif.max_read  = 254; /* maximum number of bytes allowed per serial read */
	icm20948_serif.max_write = 254; /* maximum number of bytes allowed per serial write */
	icm20948_serif.is_spi	 = false;

	inv_icm20948_reset_states(&icm_device, &icm20948_serif);

	err = icm20948_sensor_setup();
	if (err) return err;

	err = icm20948_run_selftest();
	if (err) return err;
	inv_icm20948_set_offset(&icm_device, unscaled_bias);

	 return NRF_SUCCESS;
}

uint64_t inv_icm20948_get_time_us(void)
{
	return systick_get_millis();
}

void inv_icm20948_sleep_us(int us)
{
	nrfx_coredep_delay_us(us);
}
