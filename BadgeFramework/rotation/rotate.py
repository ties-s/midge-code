import sys
from typing import Final
import pigpio
import json
from pydantic import BaseModel, ValidationError
from enum import Enum
DIR: Final = 20     # Direction GPIO Pin
STEP: Final = 12    # Step GPIO Pin
INSTRUCTION_FILE: Final = "./rotation_instructions.json"


class DirectionEnum(str, Enum):
    left = 'left'  # Counter clockwise
    right = 'right'  # Clockwise


def pulseDirection(direction: DirectionEnum) -> pigpio.pulse:
    if(direction == DirectionEnum.right):
        # Turn on
        return pigpio.pulse(1 << DIR, 0, 0)
    # Turn off
    return pigpio.pulse(0, 1 << DIR, 0)


class Instruction(BaseModel):
    freq: int
    steps: int
    dir: DirectionEnum


class InstructionList(BaseModel):
    __root__: list[Instruction]


# Connect to pigpiod daemon
pi = pigpio.pi()

# Set up pins as an output
pi.set_mode(DIR, pigpio.OUTPUT)
pi.set_mode(STEP, pigpio.OUTPUT)


MODE: Final = (14, 15, 18)   # Microstep Resolution GPIO Pins
RESOLUTION: Final = {'Full': (0, 0, 0),
                     'Half': (1, 0, 0),
                     '1/4': (0, 1, 0),
                     '1/8': (1, 1, 0),
                     '1/16': (0, 0, 1),
                     '1/32': (1, 0, 1)}
for i in range(3):
    pi.write(MODE[i], RESOLUTION['Full'][i])


def executeInstructions(instruction_list: list[Instruction]):
    """Generate ramp wave forms.
    ramp:  List of [Frequency, Steps]
    """
    pi.wave_clear()     # clear existing waves
    length = len(instruction_list)  # number of ramp levels
    wid = [-1] * length

    # Generate a wave per ramp level
    for i in range(length):
        frequency = instruction_list[i].freq
        micros = int(500000 / frequency)
        wf: list[pigpio.pulse] = []
        wf.append(pigpio.pulse(1 << STEP, 0, micros))  # pulse on
        wf.append(pulseDirection(instruction_list[i].dir))
        wf.append(pigpio.pulse(0, 1 << STEP, micros))  # pulse off
        pi.wave_add_generic(wf)
        wid[i] = pi.wave_create()

    # Generate a chain of waves
    chain = []
    for i in range(length):
        steps = instruction_list[i].steps
        x = steps & 255
        y = steps >> 8
        chain += [255, 0, wid[i], 255, 1, x, y]

    pi.wave_chain(chain)  # Transmit chain.


def verifyFile(path: str) -> InstructionList:
    f = open(path)
    instructions = json.load(f)
    if not isinstance(instructions, list):
        sys.exit("Stopping: the root element of a file is not an array")
    try:
        return InstructionList(__root__=instructions)
    except ValidationError as e:
        sys.exit(f"Stopping: {e}")


def main():
    instructionList = verifyFile(INSTRUCTION_FILE).__root__
    executeInstructions(instructionList)


if __name__ == "__main__":
    main()
