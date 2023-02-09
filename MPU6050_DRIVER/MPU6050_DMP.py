"""
MPU6050_DMP
=========================================
Micropython implementation of MPU6050_Digital_Motion_Proccessor_Driver
Author : Cagin Agirdemir
"""

from micropython import const
import time

# CHIP ADDRESS
MPU6050_AD = const(0x68)

# MPU6050 REGISTERS
PWR_MGMT_1          = const(0x6B)
SIGNAL_PATH_RESET   = const(0x68)
MOT_THR             = const(0x1F)
MOT_DUR             = const(0x20)
ZRMOT_THR           = const(0x21)
ZRMOT_DUR           = const(0x22)
ACCEL_CONFIG        = const(0x1C)
MOT_DETECT_CTRL     = const(0x69)
INT_ENABLE          = const(0x38)

# ACCEL_ON_DELAY VALUES
DELAY_3MS = const(0x30)
DELAY_2MS = const(0x10)
DELAY_2MS = const(0x20)

# ACCEL_HPF VALUES
#CutOff
HPF_NONE    = const(0x00)
HPF_5Hz     = const(0x01)
HPF_2_5Hz   = const(0x02)
HPF_1_25Hz  = const(0x03)
HPF_0_63Hz  = const(0x04)
HPF_HOLD    = const(0x07)

#MOTION AND ZERO_MOTION INTERRUPT MASK
MOT_MASK = const(0x60)


"""
MOT_DETECT_STATUS
It is proctected status until read it
Bits 7         6        5        4        3         2        1        0
Ack  MOT_XNEG  MOT_XPOS MOT_YNEG MOT_YPOS MOT_ZNEG	MOT_ZPOS Reserved MOT_ZRMOT
"""

MOT_DETECT_STATUS = const(0x61)

"""
    MPU6050_DMP_drive
    :param      i2c                 : i2c object
    :param byte motion_threshold    : every bit equal 1 mili g
    :param byte motion_duration     : every bit equal 1 millisecond
    :param byte zero_motion_thr     : every bit equal 1 mili g
    :param byte zero_motion_dur     : every bit equal 1 millisecond
    :param bool debug               : Enable debugging output
"""
class MPU6050_DMP_drive:
    def __init__(
        self, i2c,
        motion_threshold = b'\x02',
        motion_duration = b'\x05',
        zero_motion_thr = b'\x08',
        zero_motion_dur = b'\x02',
        debug=false
        ):
        self._cutoff = HPF_5Hz
        self._motion_threshold = motion_threshold
        self._motion_duration = motion_duration
        self._zero_motion_thr = zero_motion_thr
        self._zero_motion_dur = zero_motion_dur
        self._debug = debug
        self._i2c = i2c

        try:
            self._i2c.writeto_mem(MPU6050_AD, PWR_MGMT_1, b'\x00') #wakeup!
            #MOT_DETECT_CTRL SET
            ret = self._i2c.readfrom_mem(MPU6050_AD, MOT_DETECT_CTRL, 1)
            ret = int.from_bytes(ret, "big")
            ret |= DELAY_3MS
            self._i2c.writeto_mem(MPU6050_AD, MOT_DETECT_CTRL, (ret).to_bytes(1, byteorder ='big')))
            #ACCEL_CONFIG SET
            self.set_filter_cutoff()
            #THRESHOLD AND DURATION SET
            self.i2c.writeto_mem(MPU6050_AD, MOT_THR, self._motion_threshold) # set motion threshold -> 2mg 
            self.i2c.writeto_mem(MPU6050_AD, MOT_DUR, self._motion_duration) # set motion duration -> 5ms
            self.i2c.writeto_mem(MPU6050_AD, ZRMOT_THR, self._zero_motion_thr) # set motion threshold -> 8mg #for all axis
            self.i2c.writeto_mem(MPU6050_AD, ZRMOT_DUR, self._zero_motion_dur) # set motion duration -> 2ms #for all axis
            #INTERRUPT ENABLE SET
            ret = self._i2c.readfrom_mem(MPU6050_AD, INT_ENABLE, 1)
            ret = (int.from_bytes(ret, "big") | MOT_MASK)
            self._i2c.writeto_mem(MPU6050_AD, INT_ENABLE, (ret).to_bytes(1, byteorder ='big')))  
        except OSError as ex:
            for err in ex.args:
                if(err == 116):
                    print("connection error!")
                else:
                    print(err)

    def __enter__(self):
        return self

    def __exit__(self):
        pass

    """
    set_filter_cutoff
    :param byte cutoff_freq         : High_pass_filter_cutoff_frequence
    """
    def set_filter_cutoff(self, cutoff_freq = HPF_5Hz):
        self._cutoff = cutoff_freq
        try:
            ret = self._i2c.readfrom_mem(MPU6050_AD, ACCEL_CONFIG, 1)
            ret = int.from_bytes(ret, "big")
            ret |= self._cutoff
            self._i2c.writeto_mem(MPU6050_AD, ACCEL_CONFIG, (ret).to_bytes(1, byteorder ='big')))  
        except OSError as ex:
            for err in ex.args:
                if(err == 116):
                    print("connection error!")
                else:
                    print(err)
        
    """
    x_axis_check_motion
    :return int 0   no-motion
    :return int 1   forward motion in x axis
    :return int -1  backward motion in x axis
    """
    def x_axis_check_motion(self):
        try:
            self.res = self.i2c.readfrom_mem(MPU6050_AD, MOT_DETECT_STATUS, 1)
            int_val = int.from_bytes(self.res, "big")
            if(int_val & 0x01):
                return 0 # zero motion active!
            int_val>>=6 # bit shifting
            if(int_val & 0x01):
                return 1 # position x motion
            elif(int_val & 0x02):
                return -1 # negative x motion
        except OSError as ex:
            for err in ex.args:
                if(err == 116):
                    print("connection error!")
                else:
                    print(err)
