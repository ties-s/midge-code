from time import sleep
import pigpio

DIR = 20     # Direction GPIO Pin
STEP = 12    # Step GPIO Pin
SWITCH = 16  # GPIO pin of switch

# Connect to pigpiod daemon
pi = pigpio.pi()

# Set up pins as an output
pi.set_mode(DIR, pigpio.OUTPUT)
pi.set_mode(STEP, pigpio.OUTPUT)

# Set up input switch
pi.set_mode(SWITCH, pigpio.INPUT)
pi.set_pull_up_down(SWITCH, pigpio.PUD_UP)

MODE = (14, 15, 18)   # Microstep Resolution GPIO Pins
RESOLUTION = {'Full': (0, 0, 0),
              'Half': (1, 0, 0),
              '1/4': (0, 1, 0),
              '1/8': (1, 1, 0),
              '1/16': (0, 0, 1),
              '1/32': (1, 0, 1)}
for i in range(3):
    pi.write(MODE[i], RESOLUTION['Half'][i])

# # Set duty cycle and frequency
# pi.set_PWM_dutycycle(STEP, 100)  # PWM 1/2 On 1/2 Off
# pi.set_PWM_frequency(STEP, 100)  # 500 pulses per second
pi.hardware_PWM(STEP, 700, 500000)  # 800Hz 25% dutycycle

try:
    while True:
        pi.write(DIR, 0)  # Set direction
        sleep(.1)

except KeyboardInterrupt:
    print("\nCtrl-C pressed.  Stopping PIGPIO and exiting...")
finally:
    pi.set_PWM_dutycycle(STEP, 0)  # PWM off
    pi.stop()
