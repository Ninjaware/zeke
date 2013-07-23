# Kernel config

# Supported MCU models
MCU_MODEL_STM32F0 = 1
MCU_MODEL_STR912F = 2

# Supported Cores
__ARM4T__  = 1
#__ARM4TM__ = 2
#__ARM5__   = 3
#__ARM5E__  = 4
__ARM6__    = 5
__ARM6M__   = 6
#__ARM6SM__ = 7
#__ARM7M__  = 8
#__ARM7EM__ = 9
#__ARM7A__  = 10
#__ARM7R__  = 11

# Supported load average calculation periods
SCHED_LAVGPERIOD_5SEC  = 5
SCHED_LAVGPERIOD_11SEC = 11

# User configurable part begins ################################################

# Kernel configuration
configMCU_MODEL = MCU_MODEL_STM32F0
configARM_PROFILE_M = 1
configCORE = __ARM6M__

# Scheduler Selection
configSCHED_TINY = 1

# Tiny Scheduler
configSCHED_MAX_THREADS = 10      # Maximum number of threads
configFAST_FORK = 0               # Use a queue to find the next free threadId.
configSCHED_FREQ = 100u           # Scheduler frequency in Hz
configSCHED_LAVG_PER = SCHED_LAVGPERIOD_11SEC
configHEAP_BOUNDS_CHECK = 0       # Enable or disable heap bounds check
configTIMERS_MAX = 4              # Maximum number of timers available

# APP Main
configAPP_MAIN_SSIZE = 200u    # Stack size for app_main thread
configAPP_MAIN_PRI = osPriorityNormal # Priority of app_main thread

# Kernel Services
configDEVSUBSYS = 1       # Enable device driver subsystem
configPTTK91_VM = 0
