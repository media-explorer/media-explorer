/* This file was taken from scan-s2, and changed to use with w_scan.
 */

#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <linux/dvb/frontend.h>
#include "scan.h"
#include "satellites.h"
#include "diseqc.h"



/* http://www.eutelsat.com/satellites/pdf/Diseqc/Reference docs/bus_spec.pdf
 * http://www.eutelsat.com/satellites/pdf/Diseqc/associated%20docs/positioner_appli_notice.pdf
 * http://www.spaun.de/files/8df79_de_DiSEqC_fur_Techniker_.pdf
 *
 * FRAMING parity ADDRESS parity COMMAND parity DATA parity
 *
 * --added some definitions to improve readability--
 * 2009-03-16, wirbel
 */


/**************** 6.1. Framing (first) byte **********************************/
#define MASTER_CMD_NO_RESPONSE                  0xE0
#define MASTER_CMD_NO_RESPONSE_REPEATED         0xE1
#define MASTER_CMD_WITH_RESPONSE                0xE2
#define MASTER_CMD_WITH_RESPONSE_REPEATED       0xE3
#define SLAVE_REPLY_OK                          0xE4
#define SLAVE_REPLY_UNSUPPORTED                 0xE5
#define SLAVE_REPLY_PARITY_ERR                  0xE6
#define SLAVE_REPLY_CMD_UNKNOWN                 0xE7


/**************** 6.2. Address (second) byte *********************************/
#define ADDR_ANY_DEVICE                         0x00
#define ADDR_ANY_LNB                            0x10
#define ADDR_LNB                                0x11
#define ADDR_LNB_W_LOOP_THROUGH_SWITCHING       0x12
#define ADDR_SWITCHER_DC_BLOCKING               0x14
#define ADDR_SWITCHER_DC_LOOP_TROUGH            0x15
#define ADDR_SMATV                              0x18
#define ADDR_ANY_POLARISER                      0x20
#define ADDR_POLARISER_LINEAR                   0x21
#define ADDR_ANY_POSITIONER                     0x30
#define ADDR_POSITIONER_POLAR_AZIMUT            0x31
#define ADDR_POSITIONER_ELEVATION               0x32
#define ADDR_ANY_INSTALLER_AID                  0x40
#define ADDR_SIGNAL_STRENGTH_ANALOG             0x41
#define ADDR_ANY_INTELLIGENT_SLAVE_INTF         0x70
#define ADDR_INTF_SUBSCR_CONTROLLED_HEADEND     0x71


/**************** 8.3. Command Byte ******************************************/
#define CMD_RESET                               0x00
#define CMD_CLR_RESET                           0x01
#define CMD_STANDBY                             0x02
#define CMD_POWER_ON                            0x03
#define CMD_SET_CONTEND                         0x04
#define CMD_CONTEND                             0x05
#define CMD_CLEAR_CONTEND                       0x06
#define CMD_ADDRESS                             0x07
#define CMD_MOVE_C                              0x08
#define CMD_MOVE                                0x09
#define CMD_STATUS                              0x10
#define CMD_CONFIG                              0x11
#define CMD_SWITCH0                             0x14
#define CMD_SWITCH1                             0x15
#define CMD_SWITCH2                             0x16
#define CMD_SWITCH3                             0x17
#define CMD_SET_LOW_LO                          0x20
#define CMD_SET_VERT_RIGHT                      0x21
#define CMD_SET_POS_A                           0x22
#define CMD_SET_SOA                             0x23
#define CMD_SET_HIGH_LO                         0x24
#define CMD_SET_HORZ_LEFT                       0x25
#define CMD_SET_POS_B                           0x26
#define CMD_SET_SOB                             0x27
#define CMD_SET_S1A                             0x28
#define CMD_SET_S2A                             0x29
#define CMD_SET_S3A                             0x2A
#define CMD_SET_S4A                             0x2B
#define CMD_SET_S1B                             0x2C
#define CMD_SET_S2B                             0x2D
#define CMD_SET_S3B                             0x2E
#define CMD_SET_S4B                             0x2F
#define CMD_SLEEP                               0x30
#define CMD_AWAKE                               0x31
#define CMD_WR_N0_COMMITTED                     0x38
#define CMD_WR_N1_UNCOMMITTED                   0x39
#define CMD_WR_N2                               0x3A
#define CMD_WR_N3                               0x3B
#define CMD_RD_A0                               0x40
#define CMD_RD_A1                               0x41
#define CMD_WR_A0                               0x48
#define CMD_WR_A1                               0x49
#define CMD_LO_STRING                           0x50
#define CMD_LO_NOW                              0x51
#define CMD_LO_LOW                              0x52
#define CMD_LO_HIGH                             0x53
#define CMD_WR_FREQ                             0x58
#define CMD_WR_CHAN_NO                          0x59
#define CMD_HALT                                0x60
#define CMD_LIMITS_OFF                          0x63
#define CMD_RD_POS_STATUS                       0x64
#define CMD_LIMIT_EAST                          0x66
#define CMD_LIMIT_WEST                          0x67
#define CMD_DRIVE_EAST                          0x68
#define CMD_DRIVE_WEST                          0x69
#define CMD_STORE_SAT_POS                       0x6A
#define CMD_GOTO_SAT_POS_NN                     0x6B
#define CMD_GOTO_ANGLE_NN_N                     0x6E
#define CMD_SET_POSNS                           0x6F

/******************************************************************************
 * hmm, timeout is not used here - remove?
 *
 ******************************************************************************/ 
struct diseqc_cmd committed_switch_cmds[] = {
        { { { MASTER_CMD_NO_RESPONSE, ADDR_ANY_LNB, CMD_WR_N0_COMMITTED, 0xf0, 0x00, 0x00 }, 4 }, 20 },
        { { { MASTER_CMD_NO_RESPONSE, ADDR_ANY_LNB, CMD_WR_N0_COMMITTED, 0xf2, 0x00, 0x00 }, 4 }, 20 },
        { { { MASTER_CMD_NO_RESPONSE, ADDR_ANY_LNB, CMD_WR_N0_COMMITTED, 0xf1, 0x00, 0x00 }, 4 }, 20 },
        { { { MASTER_CMD_NO_RESPONSE, ADDR_ANY_LNB, CMD_WR_N0_COMMITTED, 0xf3, 0x00, 0x00 }, 4 }, 20 },
        { { { MASTER_CMD_NO_RESPONSE, ADDR_ANY_LNB, CMD_WR_N0_COMMITTED, 0xf4, 0x00, 0x00 }, 4 }, 20 },
        { { { MASTER_CMD_NO_RESPONSE, ADDR_ANY_LNB, CMD_WR_N0_COMMITTED, 0xf6, 0x00, 0x00 }, 4 }, 20 },
        { { { MASTER_CMD_NO_RESPONSE, ADDR_ANY_LNB, CMD_WR_N0_COMMITTED, 0xf5, 0x00, 0x00 }, 4 }, 20 },
        { { { MASTER_CMD_NO_RESPONSE, ADDR_ANY_LNB, CMD_WR_N0_COMMITTED, 0xf7, 0x00, 0x00 }, 4 }, 20 },
        { { { MASTER_CMD_NO_RESPONSE, ADDR_ANY_LNB, CMD_WR_N0_COMMITTED, 0xf8, 0x00, 0x00 }, 4 }, 20 },
        { { { MASTER_CMD_NO_RESPONSE, ADDR_ANY_LNB, CMD_WR_N0_COMMITTED, 0xfa, 0x00, 0x00 }, 4 }, 20 },
        { { { MASTER_CMD_NO_RESPONSE, ADDR_ANY_LNB, CMD_WR_N0_COMMITTED, 0xf9, 0x00, 0x00 }, 4 }, 20 },
        { { { MASTER_CMD_NO_RESPONSE, ADDR_ANY_LNB, CMD_WR_N0_COMMITTED, 0xfb, 0x00, 0x00 }, 4 }, 20 },
        { { { MASTER_CMD_NO_RESPONSE, ADDR_ANY_LNB, CMD_WR_N0_COMMITTED, 0xfc, 0x00, 0x00 }, 4 }, 20 },
        { { { MASTER_CMD_NO_RESPONSE, ADDR_ANY_LNB, CMD_WR_N0_COMMITTED, 0xfe, 0x00, 0x00 }, 4 }, 20 },
        { { { MASTER_CMD_NO_RESPONSE, ADDR_ANY_LNB, CMD_WR_N0_COMMITTED, 0xfd, 0x00, 0x00 }, 4 }, 20 },
        { { { MASTER_CMD_NO_RESPONSE, ADDR_ANY_LNB, CMD_WR_N0_COMMITTED, 0xff, 0x00, 0x00 }, 4 }, 20 }
};

struct diseqc_cmd uncommitted_switch_cmds[] = {
        { { { MASTER_CMD_NO_RESPONSE, ADDR_ANY_LNB, CMD_WR_N1_UNCOMMITTED, 0xf0, 0x00, 0x00 }, 4 }, 20 },
        { { { MASTER_CMD_NO_RESPONSE, ADDR_ANY_LNB, CMD_WR_N1_UNCOMMITTED, 0xf1, 0x00, 0x00 }, 4 }, 20 },
        { { { MASTER_CMD_NO_RESPONSE, ADDR_ANY_LNB, CMD_WR_N1_UNCOMMITTED, 0xf2, 0x00, 0x00 }, 4 }, 20 },
        { { { MASTER_CMD_NO_RESPONSE, ADDR_ANY_LNB, CMD_WR_N1_UNCOMMITTED, 0xf3, 0x00, 0x00 }, 4 }, 20 },
        { { { MASTER_CMD_NO_RESPONSE, ADDR_ANY_LNB, CMD_WR_N1_UNCOMMITTED, 0xf4, 0x00, 0x00 }, 4 }, 20 },
        { { { MASTER_CMD_NO_RESPONSE, ADDR_ANY_LNB, CMD_WR_N1_UNCOMMITTED, 0xf5, 0x00, 0x00 }, 4 }, 20 },
        { { { MASTER_CMD_NO_RESPONSE, ADDR_ANY_LNB, CMD_WR_N1_UNCOMMITTED, 0xf6, 0x00, 0x00 }, 4 }, 20 },
        { { { MASTER_CMD_NO_RESPONSE, ADDR_ANY_LNB, CMD_WR_N1_UNCOMMITTED, 0xf7, 0x00, 0x00 }, 4 }, 20 },
        { { { MASTER_CMD_NO_RESPONSE, ADDR_ANY_LNB, CMD_WR_N1_UNCOMMITTED, 0xf8, 0x00, 0x00 }, 4 }, 20 },
        { { { MASTER_CMD_NO_RESPONSE, ADDR_ANY_LNB, CMD_WR_N1_UNCOMMITTED, 0xf9, 0x00, 0x00 }, 4 }, 20 },
        { { { MASTER_CMD_NO_RESPONSE, ADDR_ANY_LNB, CMD_WR_N1_UNCOMMITTED, 0xfa, 0x00, 0x00 }, 4 }, 20 },
        { { { MASTER_CMD_NO_RESPONSE, ADDR_ANY_LNB, CMD_WR_N1_UNCOMMITTED, 0xfb, 0x00, 0x00 }, 4 }, 20 },
        { { { MASTER_CMD_NO_RESPONSE, ADDR_ANY_LNB, CMD_WR_N1_UNCOMMITTED, 0xfc, 0x00, 0x00 }, 4 }, 20 },
        { { { MASTER_CMD_NO_RESPONSE, ADDR_ANY_LNB, CMD_WR_N1_UNCOMMITTED, 0xfd, 0x00, 0x00 }, 4 }, 20 },
        { { { MASTER_CMD_NO_RESPONSE, ADDR_ANY_LNB, CMD_WR_N1_UNCOMMITTED, 0xfe, 0x00, 0x00 }, 4 }, 20 },
        { { { MASTER_CMD_NO_RESPONSE, ADDR_ANY_LNB, CMD_WR_N1_UNCOMMITTED, 0xff, 0x00, 0x00 }, 4 }, 20 }
};


/******************************************************************************
 * only indices for positioning cmds[] - non standardized.
 *
 ******************************************************************************/
#define DISEQC_X                        2
#define ROTOR_CMD_HALT                  0
#define ROTOR_CMD_DISABLE_LIMITS        1
#define ROTOR_CMD_SET_LIMIT_EAST        2
#define ROTOR_CMD_SET_LIMIT_WEST        3
#define ROTOR_CMD_DRIVE_EAST_CONT       4
#define ROTOR_CMD_DRIVE_EAST_STEP       5
#define ROTOR_CMD_DRIVE_WEST_STEP       6
#define ROTOR_CMD_DRIVE_WEST_CONT       7
#define ROTOR_CMD_STORE_SAT_POS         8
#define ROTOR_CMD_GOTO_SAT_POS_NN       9
#define ROTOR_CMD_RECALC_POS            10
#define ROTOR_CMD_ENABLE_LIMITS         11
#define ROTOR_CMD_GOTO_ANGLE            12
#define ROTOR_CMD_WR_COMMITTED          13

int diseqc_2_x_error = 0; // do 2.x cmds only once.

/*****************************************************************************/


int rotor_command( int frontend_fd, int cmd, int n1, int n2, int n3 ) {
        int i, err = 0;
        struct dvb_diseqc_master_cmd cmds[] = {
                { { MASTER_CMD_NO_RESPONSE, ADDR_POSITIONER_POLAR_AZIMUT, CMD_HALT,             0x00,   0x00, 0x00 }, 3 }, // 0 Stop Positioner movement
                { { MASTER_CMD_NO_RESPONSE, ADDR_POSITIONER_POLAR_AZIMUT, CMD_LIMITS_OFF,       0x00,   0x00, 0x00 }, 3 }, // 1 Disable Limits
                { { MASTER_CMD_NO_RESPONSE, ADDR_POSITIONER_POLAR_AZIMUT, CMD_LIMIT_EAST,       0x00,   0x00, 0x00 }, 3 }, // 2 Set East Limit
                { { MASTER_CMD_NO_RESPONSE, ADDR_POSITIONER_POLAR_AZIMUT, CMD_LIMIT_WEST,       0x00,   0x00, 0x00 }, 3 }, // 3 Set West Limit
                { { MASTER_CMD_NO_RESPONSE, ADDR_POSITIONER_POLAR_AZIMUT, CMD_DRIVE_EAST,       0x00,   0x00, 0x00 }, 4 }, // 4 Drive Motor East continously
                { { MASTER_CMD_NO_RESPONSE, ADDR_POSITIONER_POLAR_AZIMUT, CMD_DRIVE_EAST,       256-n1, 0x00, 0x00 }, 4 }, // 5 Drive Motor East nn steps
                { { MASTER_CMD_NO_RESPONSE, ADDR_POSITIONER_POLAR_AZIMUT, CMD_DRIVE_WEST,       256-n1, 0x00, 0x00 }, 4 }, // 6 Drive Motor West nn steps
                { { MASTER_CMD_NO_RESPONSE, ADDR_POSITIONER_POLAR_AZIMUT, CMD_DRIVE_WEST,       0x00,   0x00, 0x00 }, 4 }, // 7 Drive Motor West continously
                { { MASTER_CMD_NO_RESPONSE, ADDR_POSITIONER_POLAR_AZIMUT, CMD_STORE_SAT_POS,    n1,     0x00, 0x00 }, 4 }, // 8 Store nn
                { { MASTER_CMD_NO_RESPONSE, ADDR_POSITIONER_POLAR_AZIMUT, CMD_GOTO_SAT_POS_NN,  n1,     0x00, 0x00 }, 4 }, // 9 Goto nn
                { { MASTER_CMD_NO_RESPONSE, ADDR_POSITIONER_POLAR_AZIMUT, CMD_SET_POSNS,        n1,     n2,   n3   }, 4 }, //10 Recalculate Position
                { { MASTER_CMD_NO_RESPONSE, ADDR_POSITIONER_POLAR_AZIMUT, CMD_STORE_SAT_POS,    0x00,   0x00, 0x00 }, 4 }, //11 Enable Limits
                { { MASTER_CMD_NO_RESPONSE, ADDR_POSITIONER_POLAR_AZIMUT, CMD_GOTO_ANGLE_NN_N,  n1,     n2,   0x00 }, 5 }, //12 Gotoxx
                { { MASTER_CMD_NO_RESPONSE, ADDR_ANY_LNB,                 CMD_WR_N0_COMMITTED,  0xF4,   0x00, 0x00 }, 4 }, //13 User
        };

        for (i=0; i<DISEQC_X; ++i) {
                usleep(15*1000);
                if ((err = ioctl( frontend_fd, FE_DISEQC_SEND_MASTER_CMD, &cmds[cmd])) < 0) {
                        error("FE_DISEQC_SEND_MASTER_CMD failed, err %i\n", err);
                        break;
                        }
        }
        return err;
}

static inline void msleep(uint32_t msec) {
        struct timespec req = { msec / 1000, 1000000 * (msec % 1000) };
        while (nanosleep(&req, &req))
                ;
}

static float hex_to_float(const int bin_val) {
        int b0 = (bin_val & 0xFF00);
        int b1 = (bin_val & 0x00FF);

        return (float)  (((b0 >> 4) & 0x0f) * 1000 + (b0 & 0x0f) * 100 +
                         ((b1 >> 4) & 0x0f) * 10   + (b1 & 0x0f));
}

/******************************************************************************
 * returns the angle (0.0° .. 359.9°)
 * for a given satellite from sat_list[]
 *
 ******************************************************************************/

float rotor_angle(uint16_t channellist) {
        
        if (channellist >= sat_count()) {
                info("invalid satellite index %d\n", channellist);
                return 0;
                }

        if (sat_list[channellist].west_east_flag == WEST_FLAG)
                return 360.00 - (hex_to_float(sat_list[channellist].orbital_position) / 10);
        else
                return (hex_to_float(sat_list[channellist].orbital_position) / 10);
}

/******************************************************************************
 * positioning status bits, see "3.11. Read Positioner Status Byte (Level 2.2)" 
 *
 ******************************************************************************/

#define MOVEMENT_COMPLETE       0x80
#define SOFT_LIMITS_ENABLED     0x40
#define MOVE_WEST               0x20
#define MOTOR_RUNNING           0x10
#define SOFT_LIMIT_REACHED      0x08
#define NO_POWER                0x04
#define HARD_LIMIT_REACHED      0x02
#define NO_REF_POS              0x01

/******************************************************************************
 * retrieve positioning status from DiSEqC 2.2 rotor
 *
 * As i cannot test (no Satellite at all) and DiSEqC-2.x replies are completely
 * undescribed in linuxtv DVB API (!), this function may fail all the time.
 *
 * I also have no clue weather actually *any* driver supports diseqc-2.x
 * --wirbel 20090322
 ******************************************************************************/

int get_positioner_status (int frontend_fd, int * data) {
        int err = 0;

        struct dvb_diseqc_master_cmd  GetPosStat = {{
                MASTER_CMD_WITH_RESPONSE,
                ADDR_POSITIONER_POLAR_AZIMUT,
                CMD_RD_POS_STATUS, 0, 0, 0 }, 3};

        struct dvb_diseqc_slave_reply PosStat = {{
                0, 0, 0, 0 }, 0, 150};

        if ((err = ioctl(frontend_fd, FE_DISEQC_SEND_MASTER_CMD, &GetPosStat)) != 0) {
                verbose("%s: DiSEqC-2.2 cmd RD_POS_STATUS fails - disabling DiSEqC-2.2.\n"
                        "(this is expected, if using DiSEqC-1.2 equipment)\n",
                        __FUNCTION__);
                return err; // any DiSEqC-1.x is expected to fail
                }
        if ((err = ioctl(frontend_fd, FE_DISEQC_RECV_SLAVE_REPLY, &PosStat)) != 0) {
                verbose("%s: DiSEqC-2.2 read slave reply fails - disabling DiSEqC-2.2.\n"
                        "(this is expected, if using DiSEqC-1.2 equipment)\n",
                        __FUNCTION__);
                return err; // nowhere described.
                }
        if (PosStat.msg[0] == SLAVE_REPLY_OK) {
                if (PosStat.msg_len > 1) {
                        *data = PosStat.msg[1];
                        return 0;
                        }
                }
        info("%s: unknown error.\n", __FUNCTION__);
        return -1; //unknown error
}


#define speed_13V       1.5 //degrees per second
#define speed_18V       2.4 //degrees per second

/******************************************************************************
 * Rotate a DiSEqC 1.2 rotor from position 'from_rotor_pos' to position 'to_rotor_pos',
 * from and to are assigned to sat_list[channellist].rotor_position
 ******************************************************************************/

extern int rotate_rotor (int frontend_fd, int * from, int to, uint8_t voltage_18, uint8_t hiband) {
                
        float rotor_positioning_time; //seconds
        float rotation_angle;
        int from_satlist_index, to_satlist_index;

        //convert rotor position to sat_list index
        to_satlist_index = rotor_position_to_sat_list_index(to);

        if (to > -1) {
                if (*from != to) {
                        if (*from < 0) {
                                /* starting from unknown position, therefore 
                                 * assuming worst case: 180°
                                 * diseqc-2.2 rotor should stop earlier
                                 */
                                rotation_angle = 180;
                                info("Initializing rotor to %s (rotor position %d)\n",
                                        satellite_to_short_name(to_satlist_index),
                                        sat_list[to_satlist_index].rotor_position);
                                }
                        else {
                                //convert rotor position to sat_list index
                                from_satlist_index = rotor_position_to_sat_list_index(*from);

                                info("moving rotor from %s (rotor position %d) to %s (rotor position %d)\n",
                                        satellite_to_short_name(from_satlist_index),
                                        sat_list[from_satlist_index].rotor_position,
                                        satellite_to_short_name(to_satlist_index),
                                        sat_list[to_satlist_index].rotor_position);

                                rotation_angle = abs(rotor_angle(to_satlist_index) - rotor_angle(from_satlist_index));
                                if (rotation_angle > 180)
                                        rotation_angle = 360.0 - rotation_angle;
                                }

                        rotor_positioning_time = rotation_angle / speed_18V;
                        info("expected rotation %.2fdeg (%.1f sec)\n", rotation_angle, rotor_positioning_time);
                        //switch tone off
                        if (ioctl(frontend_fd, FE_SET_TONE, SEC_TONE_OFF) < 0) {
                                info("%s: SEC_TONE_OFF failed.\n", __FUNCTION__);
                                return -1;
                                }
                        msleep(15);
                        // high voltage for high speed rotation
                        if (ioctl(frontend_fd, FE_SET_VOLTAGE, SEC_VOLTAGE_18) < 0) {
                                info("%s: SEC_VOLTAGE_18 failed.\n", __FUNCTION__);
                                return -1;
                                }
                        msleep(15);
                        if (rotor_command(frontend_fd, ROTOR_CMD_GOTO_SAT_POS_NN, to, 0, 0)) {
                                info("%s: ROTOR_CMD_GOTO_SAT_POS_NN failed.\n", __FUNCTION__);
                                return -1;
                                }
                        else {
                                int i, status;
                                info("Rotating");
                                for (i = 0; i < (int) (rotor_positioning_time + 0.5); i++) {
                                        if ((i % 32) == 0)
                                                info("\n\t");
                                        if (diseqc_2_x_error == 0) {
                                                usleep(1000000 - 82500);
                                                if (! (diseqc_2_x_error = get_positioner_status(frontend_fd, &status))) {
                                                        if (status & MOVEMENT_COMPLETE)
                                                                break;
                                                        if (status & HARD_LIMIT_REACHED)
                                                                break;
                                                        if ((status & MOTOR_RUNNING) != MOTOR_RUNNING)
                                                                break;
                                                        }
                                                }
                                        else
                                                usleep(1000000);
                                        info("%d ", (int) rotor_positioning_time - i);
                                        }
                                info(" completed.\n");
                                }
                        *from = to;
                        }
                // correct tone and voltage
                if (ioctl(frontend_fd, FE_SET_TONE, hiband ? SEC_TONE_ON : SEC_TONE_OFF)) {
                        info("%s: FE_SET_TONE failed.\n", __FUNCTION__);
                        return -1;
                        }
                msleep(15);
                if (ioctl(frontend_fd, FE_SET_VOLTAGE, voltage_18)) {
                        info("%s: FE_SET_VOLTAGE failed.\n", __FUNCTION__);
                        return -1;
                        }
                msleep(15);
                }
        else
                info("warn: to position < 0, ignored.\n");
        return 0;
}


int diseqc_send_msg (int fd, fe_sec_voltage_t v, struct diseqc_cmd **cmd,
                                         fe_sec_tone_mode_t t, fe_sec_mini_cmd_t b)
{
        int err;

        if ((err = ioctl(fd, FE_SET_TONE, SEC_TONE_OFF))) {
                info("%s: SEC_TONE_OFF failed.\n", __FUNCTION__);
                return err;
                }
        if ((err = ioctl(fd, FE_SET_VOLTAGE, v))) {
                info("%s: FE_SET_VOLTAGE failed.\n", __FUNCTION__);
                return err;
                }
        msleep(15);

        while (*cmd) {
                debug("DiSEqC: %02x %02x %02x %02x %02x %02x\n",
                        (*cmd)->cmd.msg[0], (*cmd)->cmd.msg[1],
                        (*cmd)->cmd.msg[2], (*cmd)->cmd.msg[3],
                        (*cmd)->cmd.msg[4], (*cmd)->cmd.msg[5]);

                if ((err = ioctl(fd, FE_DISEQC_SEND_MASTER_CMD, &(*cmd)->cmd))) {
                        info("%s: FE_DISEQC_SEND_MASTER_CMD failed.\n", __FUNCTION__);
                        return err;
                        }
                cmd++;
        }

        msleep(15);

        if ((err = ioctl(fd, FE_DISEQC_SEND_BURST, b))) {
                info("%s: FE_DISEQC_SEND_MASTER_CMD failed.\n", __FUNCTION__);
                return err;
                }
        msleep(15);

        if ((err = ioctl(fd, FE_SET_TONE, t))) {
                info("%s: FE_SET_TONE failed.\n", __FUNCTION__);
                return err;
                }                
        msleep(15);

        return err;
}

int setup_switch (int frontend_fd, int switch_pos, int voltage_18, int hiband, int uncommitted_switch_pos)
{
        int i;
        int err;
        struct diseqc_cmd *cmd[2] = { NULL, NULL };

        i = uncommitted_switch_pos;

        verbose("DiSEqC: uncommitted switch pos %i\n", uncommitted_switch_pos);
        if (i < 0 || i >= (int) (sizeof(uncommitted_switch_cmds)/sizeof(struct diseqc_cmd)))
                return -EINVAL;

        cmd[0] = &uncommitted_switch_cmds[i];

        diseqc_send_msg (frontend_fd,
                voltage_18 ? SEC_VOLTAGE_18 : SEC_VOLTAGE_13,
                cmd,
                hiband ? SEC_TONE_ON : SEC_TONE_OFF,
                switch_pos % 2 ? SEC_MINI_B : SEC_MINI_A);

        i = 4 * switch_pos + 2 * hiband + (voltage_18 ? 1 : 0);

        verbose("DiSEqC: switch pos %i, %sV, %sband (index %d)\n",
                switch_pos, voltage_18 ? "18" : "13", hiband ? "hi" : "lo", i);

        if (i < 0 || i >= (int) (sizeof(committed_switch_cmds)/sizeof(struct diseqc_cmd)))
                return -EINVAL;

        cmd[0] = &committed_switch_cmds[i];

        err = diseqc_send_msg (frontend_fd,
                voltage_18 ? SEC_VOLTAGE_18 : SEC_VOLTAGE_13,
                cmd,
                hiband ? SEC_TONE_ON : SEC_TONE_OFF,
                switch_pos % 2 ? SEC_MINI_B : SEC_MINI_A);

        return err;
}

