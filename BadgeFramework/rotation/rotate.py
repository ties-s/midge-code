from typing import Final, TypedDict
import pigpio

DIR: Final = 20     # Direction GPIO Pin
STEP: Final = 12    # Step GPIO Pin


class RampInstruction(TypedDict):
    freq: int
    steps: int


# Connect to pigpiod daemon
pi = pigpio.pi()

# Set up pins as an output
pi.set_mode(DIR, pigpio.OUTPUT)
pi.set_mode(STEP, pigpio.OUTPUT)


MODE:Final = (14, 15, 18)   # Microstep Resolution GPIO Pins
RESOLUTION: Final = {'Full': (0, 0, 0),
                     'Half': (1, 0, 0),
                     '1/4': (0, 1, 0),
                     '1/8': (1, 1, 0),
                     '1/16': (0, 0, 1),
                     '1/32': (1, 0, 1)}
for i in range(3):
    pi.write(MODE[i], RESOLUTION['Full'][i])


def generate_ramp(instruction_list: list[RampInstruction]):
    """Generate ramp wave forms.
    ramp:  List of [Frequency, Steps]
    """
    pi.wave_clear()     # clear existing waves
    length = len(instruction_list)  # number of ramp levels
    wid = [-1] * length

    # Generate a wave per ramp level
    for i in range(length):
        frequency = instruction_list[i]["freq"]
        micros = int(500000 / frequency)
        wf: list[pigpio.pulse] = []
        wf.append(pigpio.pulse(1 << STEP, 0, micros))  # pulse on
        wf.append(pigpio.pulse(0, 1 << STEP, micros))  # pulse off
        pi.wave_add_generic(wf)
        wid[i] = pi.wave_create()

    # Generate a chain of waves
    chain = []
    for i in range(length):
        steps = instruction_list[i]["steps"]
        x = steps & 255
        y = steps >> 8
        chain += [255, 0, wid[i], 255, 1, x, y]

    pi.wave_chain(chain)  # Transmit chain.


generate_ramp([{'freq': 1000, 'steps': 200}])
