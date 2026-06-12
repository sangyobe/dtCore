#include <signal.h>
#include <thread>
#include <random>
#include <dtCore/dtLog>
#include <dtCore/dtThread>

// Layout 0 ("Layout#0") group indices
static constexpr int L0_GRP_THREAD = 0;
static constexpr int L0_GRP_TASK   = 1;
static constexpr int L0_GRP_R_JOINT = 2;
static constexpr int L0_GRP_L_JOINT = 3;

// Layout 1 ("Layout#1") group indices
static constexpr int L1_GRP_THREAD = 0;
static constexpr int L1_GRP_ECAT   = 1;
static constexpr int L1_GRP_GRIP   = 2;
static constexpr int L1_GRP_OBJ    = 3;
static constexpr int L1_GRP_BARCODE = 4;
static constexpr int L1_GRP_ODOM   = 5; // header-less
static constexpr int L1_GRP_PLUD   = 6;
static constexpr int L1_GRP_NAVI   = 7; // header-less
static constexpr double RAD2DEG = 57.295779513; //! 180.0f/M_PI
static constexpr double DEG2RAD = 0.0174532925; //! M_PI/180.0f

static void SetupTuiLayout0();
static void SetupTuiLayout1();
static void UpdateThreadState(int layout);
static void UpdateRobotTaskData();
static void UpdateRobotJointData();
static void UpdateGripperData();
static void UpdatePerceivedObject();
static void UpdateQRInfo();
static void UpdatePludState();
static void UpdateEtherCAT();
static void KeyHandle(char c);

static std::random_device rd;
static int random_int_list[50] = {0,};
static double random_double_list[50] = {0,};
static bool bRun = true;

static void CatchSignal(int sig)
{
    LOG(info).printf("catch signal: %d\n", sig);
    switch (sig)
    {
    case SIGINT: // SIGINT
        bRun = false;
        break;
    }
}

int main(int argc, const char **argv)
{
    signal(SIGTERM, CatchSignal); // kill command
    signal(SIGINT, CatchSignal);  // keyboard interrupt, Ctrl + c
    
    std::string logName = "example_log_tui";
    std::string logFilepath = "";
    int logLevelVal = dt::Log::LogLevel::trace;
    bool enableTui = true;
    uint32_t debug_cnt = 0;

    dt::Log::Initialize(logName, logFilepath, enableTui);
    dt::Log::SetLogLevel(static_cast<dt::Log::LogLevel>(logLevelVal));
    // FlushOn is not necessary for RT Log because it is designed to be non-blocking and thread-safe automatically flushes on periodic intervals.
    dt::Log::FlushOn(dt::Log::LogLevel::info);

    if (enableTui)
    {
        SetupTuiLayout0();
        SetupTuiLayout1();
    }

    LOG(info) << "Program started" << (enableTui ? " (TUI mode)" : " (Console mode)");

    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> distrib(1, INT32_MAX);
    std::uniform_real_distribution<double> distrib_double(1, 10);

    while (bRun)
    {
        for (int i = 0; i < 50; i++) 
        {
            random_int_list[i] = distrib(gen);
            random_double_list[i] = distrib_double(gen);
        }
        
        UpdateThreadState(0);
        UpdateRobotTaskData();
        UpdateRobotJointData();

        UpdateThreadState(1);
        UpdateEtherCAT();
        UpdateGripperData();
        UpdatePerceivedObject();
        UpdateQRInfo();
        UpdatePludState();

        // 10ms delay
        dt::Thread::SleepForMillis(10);

        // key
        if (enableTui) {
            char key = TUI_GET_PENDING_KEY();
            if (key != 0)
            {
                KeyHandle(key);
            }
        }

        debug_cnt++;
        if (debug_cnt % 100 == 0) {
            LOG(info).format("debug_cnt: {}", debug_cnt);
        }
    }

    dt::Log::Terminate();
    return 0;
}

void SetupTuiLayout0()
{
    TUI_SET_LAYOUT_NAME(0, "Layout#1");

    TUI_SET_GROUP(0, L0_GRP_THREAD, "RT Thread State");
    // TUI_SET_GROUP(0, L0_GRP_OBJ, "Object State", "(mm, deg)");
    // TUI_SET_GROUP(0, L0_GRP_QR, "QR State", "(mm, deg)");
    TUI_SET_GROUP(0, L0_GRP_TASK, "Task State", "(m, deg, dps)");
    // TUI_SET_GROUP(0, L0_GRP_PLUD, "Plud State", "[status]", "desPos", "actPos", "absPos", "actVel", "tgtTPU", "tgtTor");
    TUI_SET_GROUP(0, L0_GRP_R_JOINT, "Right Arm Joint", "state", "desPos", "actPos", "absPos", "actVel", "tgtTPU", "tgtTor");
    TUI_SET_GROUP(0, L0_GRP_L_JOINT, "Left Arm Joint", "state", "desPos", "actPos", "absPos", "actVel", "tgtTPU", "tgtTor");
}

void SetupTuiLayout1()
{
    TUI_SET_LAYOUT_NAME(1, "Layout#2");

    TUI_SET_GROUP(1, L1_GRP_THREAD, "RT Thread State");
    TUI_SET_GROUP(1, L1_GRP_ECAT, "EtherCAT Master");
    TUI_SET_GROUP(1, L1_GRP_GRIP, "Gripper State", "width", "pose", "ready", "fault", "motion", "contact");
    TUI_SET_GROUP(1, L1_GRP_OBJ, "Object State");
    TUI_SET_GROUP(1, L1_GRP_BARCODE, "Barcode / QR State");
    TUI_SET_GROUP_NO_HDR(1, L1_GRP_ODOM);
    TUI_SET_GROUP(1, L1_GRP_PLUD, "PLUD Joints", "[status]", "desPos", "actPos", "absPos", "actVel", "tgtTPU", "tgtTor", "error");
    TUI_SET_GROUP_NO_HDR(1, L1_GRP_NAVI);
}

void UpdateThreadState(int layout)
{
    int grp = (layout == 0) ? L0_GRP_THREAD : L1_GRP_THREAD;
    TUI_SET_TEXT_ROW_FMT(layout, grp, 0, "Ctrl",
                        "period: %6.3f ms   load: %6.3f ms   maxLoad: %6.3f ms   overrun: %d",
                        random_double_list[0],
                        random_double_list[1],
                        random_double_list[2],
                        random_int_list[0]);
}

void UpdateRobotTaskData()
{
    TUI_SET_TEXT_ROW_FMT(0, L0_GRP_TASK, 0, "Right",
                         "Pos: %+8.3f %+8.3f %+8.3f   Vel: %+8.3f %+8.3f %+8.3f",
                         random_double_list[0], random_double_list[1], random_double_list[2],
                         random_double_list[3], random_double_list[4], random_double_list[5]);

    TUI_SET_TEXT_ROW_FMT(0, L0_GRP_TASK, 1, "     ",
                         "RPY: %+8.3f %+8.3f %+8.3f  AngV: %+8.3f %+8.3f %+8.3f",
                         random_double_list[6] * RAD2DEG,
                         random_double_list[7] * RAD2DEG,
                         random_double_list[8] * RAD2DEG,
                         random_double_list[9] * RAD2DEG,
                         random_double_list[10] * RAD2DEG,
                         random_double_list[11] * RAD2DEG);

    TUI_SET_TEXT_ROW_FMT(0, L0_GRP_TASK, 2, "Left ",
                         "Pos: %+8.3f %+8.3f %+8.3f   Vel: %+8.3f %+8.3f %+8.3f",
                         random_double_list[12], random_double_list[13], random_double_list[14],
                         random_double_list[15], random_double_list[16], random_double_list[17]);

    TUI_SET_TEXT_ROW_FMT(0, L0_GRP_TASK, 3, "     ",
                         "RPY: %+8.3f %+8.3f %+8.3f  AngV: %+8.3f %+8.3f %+8.3f",
                         random_double_list[18] * RAD2DEG,
                         random_double_list[19] * RAD2DEG,
                         random_double_list[20] * RAD2DEG,
                         random_double_list[21] * RAD2DEG,
                         random_double_list[22] * RAD2DEG,
                         random_double_list[23] * RAD2DEG);
}

void UpdateRobotJointData()
{
    const char *jointName[] = {"RB", "R1", "R2", "R3", "R4", "R5", "R6", "R7",
                                "LB", "L1", "L2", "L3", "L4", "L5", "L6", "L7"};

    for (int i = 0; i < 8; i++)
    {
        double u = (i == 0) ? 1000.0f : RAD2DEG;
        TUI_SET_ROW_COLS(0, L0_GRP_R_JOINT, i, jointName[i],
            TUI_COL("0x%04X", random_int_list[i]),
            TUI_COL("%+8.1f", random_double_list[i] * u),
            TUI_COL("%+8.1f", random_double_list[i] * u),
            TUI_COL("%+8.1f", random_double_list[i] * u),
            TUI_COL("%+8.1f", random_double_list[i] * u),
            TUI_COL("%+8d",   random_int_list[i]),
            TUI_COL("%+8.1f", random_double_list[i]));
    }

    for (int i = 0; i < 8; i++)
    {
        double u = (i == 0) ? 1000.0f : RAD2DEG;
        TUI_SET_ROW_COLS(0, L0_GRP_L_JOINT, i, jointName[8 + i],
            TUI_COL("0x%04X", random_int_list[i]),
            TUI_COL("%+8.1f", random_double_list[i] * u),
            TUI_COL("%+8.1f", random_double_list[i] * u),
            TUI_COL("%+8.1f", random_double_list[i] * u),
            TUI_COL("%+8.1f", random_double_list[i] * u),
            TUI_COL("%+8d",   random_int_list[i]),
            TUI_COL("%+8.1f", random_double_list[i]));
    }
}

void UpdateGripperData()
{
    TUI_SET_ROW(1, L1_GRP_GRIP, 0, "Right", "%d",
                    random_int_list[10],
                    random_int_list[11],
                    random_int_list[12] & (0x01 << 0) ? 1 : 0,
                    random_int_list[12] & (0x01 << 1) ? 1 : 0,
                    random_int_list[12] & (0x01 << 2) ? 1 : 0,
                    random_int_list[12] & (0x01 << 3) ? 1 : 0);

    TUI_SET_ROW(1, L1_GRP_GRIP, 1, "Left ", "%d",
                    random_int_list[13],
                    random_int_list[14],
                    random_int_list[15] & (0x01 << 0) ? 1 : 0,
                    random_int_list[15] & (0x01 << 1) ? 1 : 0,
                    random_int_list[15] & (0x01 << 2) ? 1 : 0,
                    random_int_list[15] & (0x01 << 3) ? 1 : 0);
}

void UpdatePerceivedObject()
{
    constexpr int obj_count_max = 1;
    int row = 0;

    for (int i = 0; i < 4 && row < obj_count_max; i++)
    {
        TUI_SET_TEXT_ROW_FMT(1, L1_GRP_OBJ, row++, "Obj",
                             "[%4d] Pos:%+8.3f,%+8.3f,%+8.3f   RPY:%+8.3f,%+8.3f,%+8.3f",
                             random_int_list[16],
                             random_double_list[25],
                             random_double_list[26],
                             random_double_list[27],
                             random_double_list[28] * RAD2DEG,
                             random_double_list[29] * RAD2DEG,
                             random_double_list[30] * RAD2DEG);
    }

    for (int i = row; i < obj_count_max; i++)
        TUI_SET_TEXT_ROW(1, L1_GRP_OBJ, i, "Obj", "---");
}

void UpdateQRInfo()
{
    // Barcode row is placed after object rows
    constexpr int obj_count_max = 1;
    constexpr int qr_count_max  = 3;
    int row = obj_count_max;

    TUI_SET_TEXT_ROW_FMT(1, L1_GRP_BARCODE, row++, "BC ", "%s", "test barcode#1");

    int qr_count = 0;
    for (int i = 0; i < 3 && qr_count < qr_count_max; i++) {
        TUI_SET_TEXT_ROW_FMT(1, L1_GRP_BARCODE, row++, "QR ",
                             "[%4d] Pos:%+8.3f,%+8.3f,%+8.3f   EZYX:%+8.3f,%+8.3f,%+8.3f",
                             random_int_list[17],
                             random_double_list[31],
                             random_double_list[32],
                             random_double_list[33],
                             random_double_list[34] * RAD2DEG,
                             random_double_list[35] * RAD2DEG,
                             random_double_list[36] * RAD2DEG);

        TUI_SET_TEXT_ROW_FMT(1, L1_GRP_BARCODE, row++, "   ",
                             "       Vec:%+8.3f,%+8.3f,%+8.3f   %s",
                             random_double_list[37],
                             random_double_list[38],
                             random_double_list[39],
                             "test barcode...");
        qr_count++;
    }

    for (int i = qr_count; i < qr_count_max; i++) {
        TUI_SET_TEXT_ROW(1, L1_GRP_BARCODE, row++, "QR ", "---");
        TUI_SET_TEXT_ROW(1, L1_GRP_BARCODE, row++, "   ", "");
    }
}

void UpdatePludState()
{
    const char *jointName[8] = {"S1", "W1", "S2", "W2", "S3", "W3", "S4", "W4"};

    TUI_SET_TEXT_ROW_FMT(1, L1_GRP_ODOM, 0, "Odom",
                         "Vx: %+7.3f m/s   Vy: %+7.3f m/s   Vth: %+5.0f deg/s",
                         random_double_list[40],
                         random_double_list[41],
                         random_double_list[42] * RAD2DEG);

    int row = 0;
    for (int i = 0; i < 8; i = i + 2) // steer joints
    {
        TUI_SET_ROW_COLS(1, L1_GRP_PLUD, row++, jointName[i],
            TUI_COL("0x%04X", random_int_list[i]),
            TUI_COL("%+8.1f", random_double_list[i] * RAD2DEG),
            TUI_COL("%+8.1f", random_double_list[i] * RAD2DEG),
            TUI_COL("%+8.1f", random_double_list[i] * RAD2DEG),
            TUI_COL("%+8.1f", random_double_list[i] * RAD2DEG),
            TUI_COL("%+8d",   random_int_list[i]),
            TUI_COL("%+8.1f", random_double_list[i]),
            TUI_COL("0x%04X", random_int_list[i]));
    }

    for (int i = 1; i < 8; i = i + 2) // wheel joints
    {
        TUI_SET_ROW_COLS(1, L1_GRP_PLUD, row++, jointName[i],
            TUI_COL("0x%04X", random_int_list[i]),
            TUI_COL("%+8.1f", random_double_list[i] * RAD2DEG),
            TUI_COL("%+8.1f", random_double_list[i] * RAD2DEG),
            TUI_COL("%+8.1f", random_double_list[i] * RAD2DEG),
            TUI_COL("%+8.1f", random_double_list[i] * RAD2DEG),
            TUI_COL("%+8d",   random_int_list[i]),
            TUI_COL("%+8.1f", random_double_list[i]),
            TUI_COL("0x%04X", random_int_list[i]));
    }

    TUI_SET_TEXT_ROW_FMT(1, L1_GRP_NAVI, 0, "Navi Command",
        "[x] %+6.3f m/s   [y] %+6.3f m/s   [w]: %+5.1f deg/s", 
        0.0, 0.0, 0.0);
}

void UpdateEtherCAT()
{
    char strEcLink[2][5];
    char strEcAlState[2][13];
    char strEcWcState[2][13];
    // dummy data
    int _link = 1;          // Up
    int _al_status = 8;     // OP
    int _slave_num = 24;
    int _wc_state = 2;      // Complete
    int _working_count = 72;    // Normal

    if (_link == 1)
        sprintf(strEcLink[0], "  up");
    else
        sprintf(strEcLink[0], "down");

    if (_al_status == 1)
        sprintf(strEcAlState[0], "INIT(0x01)  ");
    else if (_al_status == 2)
        sprintf(strEcAlState[0], "PREOP(0x02) ");
    else if (_al_status == 4)
        sprintf(strEcAlState[0], "SAFEOP(0x04)");
    else if (_al_status == 8)
        sprintf(strEcAlState[0], "OP(0x08)    ");
    else
        sprintf(strEcAlState[0], "0x%02X        ", _al_status);

    if (_wc_state == 0)
        sprintf(strEcWcState[0], "No exchanged");
    else if (_wc_state == 1)
        sprintf(strEcWcState[0], "Incomplete  ");
    else if (_wc_state == 2)
        sprintf(strEcWcState[0], "Complete    ");
    else
        sprintf(strEcWcState[0], "Unknown     ");
    
    TUI_SET_TEXT_ROW_FMT(1, L1_GRP_ECAT, 0, " ", "Link is %s\tSlave: %02d\tAL State: %s",
        strEcLink[0], _slave_num, strEcAlState[0]);
    TUI_SET_TEXT_ROW_FMT(1, L1_GRP_ECAT, 1, " ", "WC: %02d\t\tState: %s",
        _working_count, strEcWcState[0]);
}

void KeyHandle(char c)
{
    LOG(debug) << "key input: " << c;
    switch (c)
    {
    case 'q':
        bRun = false;
        break;
    }
}