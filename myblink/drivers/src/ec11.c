/**
 * @file ec11.c
 * @author your name (you@domain.com)
 * @brief gpio双边沿中断+软件判断4倍频识别ec11方向,每旋转一格编码器输出 4 个脉冲
 * （标准 EC11 一格通常输出 2 个脉冲，4 倍频后精度提升一倍），标准 EC11 每旋转一格，A/B 两相各产生 1 
 * 个上升沿 + 1 个下降沿（共 4 个边沿），4 倍频就是对这 4 个边沿都做计数，而非仅检测单相信号的 1 个边沿:
 * ---------------------------------------------------------------------
 * 旋转阶段	A相电平	B相电平	边沿事件  4倍频计数
 * 初始	      1    	1	   -	       0
 * 顺时针1	  0	    1	   A下降沿	   +1
 * 顺时针2	  0	    0	   B下降沿	   +1
 * 顺时针3	  1	    0	   A上升沿	   +1
 * 顺时针4	  1	    1	   B上升沿	   +1
 * ---------------------------------------------------------------------
 * 标准 2 倍频：仅检测 A 相的上升 / 下降沿（计 2 次）；
 * 4 倍频：检测 A/B 两相的所有上升 / 下降沿（计 4 次）。
 * ---------------------------------------------------------------------
 * @version 0.1
 * @date 2026-01-01
 * 
 * @copyright Copyright (c) 2026
 * 
 */
#include "ec11.h"
#include "bsp.h"
#include "input_process.h"
#include "signals.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include <stdint.h>
#include "logic.h"

#define EC11_KEY_PIN 23
#define EC11_A_PIN 28
#define EC11_B_PIN 29

#define KEY_BUF_SIZE 4
// 按键消抖时间
#define KEY_DEBOUNCING_TIME_10MS 10
// 双击阈值
#define KEY_CONTINUE_TIME_200MS 200
// 单击识别长按
#define KEY_LONG_CLICK_TIME_700MS 700
// 
#define EC11_ROTATE_POLL_INTERVAL 1
#define EC11_KEY_POLL_INTERVAL 10
// 没有硬件消抖的时候使用软件消抖
// #define ROTATE_DEBOUNCE_TIME_5MS 5 
#define ROTATE_DEBOUNCE_TIME_5MS 2 

typedef struct {
    QActive super;
    uint32_t lastRotateTick; // 旋转去抖
    uint8_t lastKeyLevel;
    uint32_t lastPressTick; // 长按判断
    uint8_t clickCount;  // 连续按下的计数
    uint32_t lastClickTick;   // 识别到单击时的 tick，判断双击用
    uint8_t lastAB; // 上一次A相和B相的值：(A << 1) | B
    int16_t rotatePosition; // ec11 旋转的位置计数
    bool hasRotate; // ec11 是否旋转
    uint8_t currentKeyState; // 当前按键状态
    QTimeEvt timeEvtKeyPoll; // 按键轮询的定时器
    QTimeEvt timeEvtDebouncing; // 去抖定时器
    QTimeEvt timeEvtRotatePoll; // 旋转轮询定时器
} Ec11;

static Ec11 g_Instance;
QActive *g_Ec11 = &g_Instance.super;

/**
 * @brief 
 * 等价 4x4 矩阵（行=上一次AB值，列=当前AB值，值=旋转方向）：
 * 当前AB →        00   01   10   11
 * int8_t qem_matrix[4][4] = {
 *     上一次AB=00 { 0,  1, -1,  0 },
 *     上一次AB=01 {-1,  0,  0,  1 },
 *     上一次AB=10 { 1,  0,  0, -1 },
 *     上一次AB=11 { 0, -1,  1,  0 }
 * };
 */
static const int8_t g_Qem[] = {
    0, 1, -1, 0,
    -1, 0, 0, 1,
    1, 0, 0, -1,
    0, -1, 1, 0
};

static uint8_t Ec11ReadAB()
{
    return (gpio_get(EC11_A_PIN) << 1) | (gpio_get(EC11_B_PIN));
}

static uint32_t Ec11GetTick()
{
    return us_to_ms(time_us_32());
    // return to_ms_since_boot(get_absolute_time());
}

static bool Ec11RotateDebounce(Ec11 *me)
{
    uint32_t now = Ec11GetTick();
    if (now - me->lastRotateTick < ROTATE_DEBOUNCE_TIME_5MS) {
        return false; // 消抖中
    }
    me->lastRotateTick = now;
    return true;
}

const char *GetKeyName(uint8_t keyIndex)
{
    static const char *keyIndexNameArray[] = {"key0", "ec11_key"};

    uint8_t size = sizeof(keyIndexNameArray) / sizeof(keyIndexNameArray[0]);
    if (keyIndex >= size) {
        return NULL;
    }
    return keyIndexNameArray[keyIndex];
}

// EC11_KEY_INVALID = 0,
// EC11_KEY_PRESS, // 按下
// EC11_KEY_CONTINUE_CLICK,	 // 单击（按下后释放触发）
// EC11_KEY_DOUBLE_CLICK, // 双击
// EC11_KEY_LONG_CLICK, // 长按
// EC11_KEY_LEFT_ROTATE, // 向左旋转一下
// EC11_KEY_RIGHT_ROTATE, // 向右旋转一下
// EC11_KEY_PRESS_LEFT_ROTATE, // 按住向左旋转一下
// EC11_KEY_PRESS_RIGHT_ROTATE, // 按住向右旋转一下
// EC11_DEBUG
const char *GetKeyEventName(uint8_t keyEvent)
{
    static const char *keyStateNameArray[] = {
        "invalid", "press", "continue click", "double_click",
        "long_click", "left_rotate", "right_rotate", "press_left_rotate",
        "press_right_rotate", "ec11_debug"};

    uint8_t size = sizeof(keyStateNameArray) / sizeof(keyStateNameArray[0]);
    if (keyEvent >= size) {
        return NULL;
    }
    return keyStateNameArray[keyEvent];
}

static QState Ec11Init(Ec11 *me, const void *arg);
static QState Ec11RotatePoll(Ec11 *me, const QEvt *e);
static QState Ec11Idle(Ec11 *me, const QEvt *e);
static QState Ec11ClickPressDebouncing(Ec11 *me, const QEvt *e);
static QState Ec11ClickReleaseDebouncing(Ec11 *me, const QEvt *e);
static QState Ec11ClickRelease(Ec11 *me, const QEvt *e);
static QState Ec11ContinueClickPress(Ec11 *me, const QEvt *e);
static QState Ec11ContinueClickPressDebouncing(Ec11 *me, const QEvt *e);
static QState Ec11ContinueClickReleaseDebouncing(Ec11 *me, const QEvt *e);
static QState Ec11ContinueClickRelease(Ec11 *me, const QEvt *e);
static uint8_t Ec11ReadAB();

void Ec11Ctor(void) 
{
    Ec11 *ec11 = &g_Instance;
    QActive_ctor(&ec11->super, Q_STATE_CAST(Ec11Init));
    QTimeEvt_ctorX(&ec11->timeEvtKeyPoll, &ec11->super, SIGNAL_KEY_POLL, 0U);
    QTimeEvt_ctorX(&ec11->timeEvtDebouncing, &ec11->super, SIGNAL_DEBOUNCING, 0U);
    QTimeEvt_ctorX(&ec11->timeEvtRotatePoll, &ec11->super, SIGNAL_ROTATE_POLL, 0U);
}

static void Ec11Reset(Ec11 *me)
{
    me->lastRotateTick = 0;
    me->lastKeyLevel = 1;
    me->lastPressTick = 0;
    me->clickCount = 0;
    me->lastClickTick = 0;
    me->lastAB = Ec11ReadAB();
    me->rotatePosition = 0;
    me->hasRotate = false;
}

QState Ec11Init(Ec11 *me, const void *arg)
{
    Q_UNUSED_PAR(me);
    Q_UNUSED_PAR(arg);
    gpio_init_mask(1 << EC11_KEY_PIN | 1 << EC11_A_PIN | 1 << EC11_B_PIN);
    gpio_set_dir_masked(1 << EC11_KEY_PIN | 1 << EC11_A_PIN | 1 << EC11_B_PIN, GPIO_IN);
    gpio_pull_up(EC11_KEY_PIN);
    gpio_pull_up(EC11_A_PIN);
    gpio_pull_up(EC11_B_PIN);
    QTimeEvt_armX(&me->timeEvtRotatePoll, EC11_ROTATE_POLL_INTERVAL, EC11_ROTATE_POLL_INTERVAL);
    return Q_TRAN(Ec11Idle);
}

QState Ec11RotatePoll(Ec11 *me, const QEvt *e)
{
    QState status = Q_HANDLED();
    switch (e->sig)
    {
    case SIGNAL_ROTATE_POLL:
    {
        static uint32_t count = 0;
        Logic1Xor();
        if (count++ % 500 == 0) {
            InputEvent *inputEvent = Q_NEW(InputEvent, SIGNAL_INPUT);
            inputEvent->data.qeueueMin = QActive_getQueueMin(me->super.prio);
            inputEvent->key = EC11_DEBUG;
            QACTIVE_POST(g_InputProcess, &inputEvent->super, &me->super);
        }
        uint8_t currentAB = Ec11ReadAB();
        uint8_t index = (me->lastAB << 2) | currentAB;
        int8_t movement = g_Qem[index & 0x0F];
        me->lastAB = currentAB;

        // 旋转消抖+有效移动判断
        if (movement != 0 && Ec11RotateDebounce(me)) {
            me->rotatePosition += movement;
            me->hasRotate = true;

            InputEvent *inputEvent = Q_NEW(InputEvent, SIGNAL_INPUT);
            inputEvent->data.rotatePosition = me->rotatePosition;
            if (movement > 0) {
                inputEvent->key = (me->lastPressTick != 0) ? EC11_KEY_PRESS_RIGHT_ROTATE : EC11_KEY_RIGHT_ROTATE;
            } else {
                inputEvent->key = (me->lastPressTick != 0) ? EC11_KEY_PRESS_LEFT_ROTATE : EC11_KEY_LEFT_ROTATE;
            }
            QACTIVE_POST(g_InputProcess, &inputEvent->super, &me->super);
        }
        status = Q_HANDLED();
        break;
    }
    default:
        status = Q_SUPER(QHsm_top);
        break;
    }

    return status;
}

/**
 * @brief 识别按下
 * 
 * @param me 
 * @param e 
 * @return QState 
 */
QState Ec11Idle(Ec11 *me, const QEvt *e)
{
    QState status = Q_HANDLED();
    switch (e->sig)
    {
    case Q_ENTRY_SIG:
        Ec11Reset(me);
        QTimeEvt_armX(&me->timeEvtKeyPoll, EC11_KEY_POLL_INTERVAL, EC11_KEY_POLL_INTERVAL);
        status = Q_HANDLED();
        break;
    case Q_EXIT_SIG:
        QTimeEvt_disarm(&me->timeEvtKeyPoll);
        status = Q_HANDLED();
        break;
    case SIGNAL_KEY_POLL:
        if (!gpio_get(EC11_KEY_PIN)) {
            status = Q_TRAN(Ec11ClickPressDebouncing);
        } else {
            status = Q_HANDLED();
        }
        break;
    default:
        status = Q_SUPER(Ec11RotatePoll); // 在父状态 Ec11RotatePoll 中进行旋转的轮询
        break;
    }

    return status;
}

/**
 * @brief 按下去抖
 * 
 * @param me 
 * @param e 
 * @return QState 
 */
QState Ec11ClickPressDebouncing(Ec11 *me, const QEvt *e)
{
    QState status = Q_HANDLED();
    switch (e->sig)
    {
    case Q_ENTRY_SIG:
        if (gpio_get(EC11_KEY_PIN)) { // 如果是高电平则提前退出
            status = Q_TRAN(Ec11Idle);
        } else {
            QTimeEvt_armX(&me->timeEvtDebouncing, KEY_DEBOUNCING_TIME_10MS, 0U); // 启动按下去抖逻辑
            status = Q_HANDLED();
        }
        break;
    case SIGNAL_DEBOUNCING:
        if (!gpio_get(EC11_KEY_PIN)) { // 按下去抖成功
            me->lastPressTick = Ec11GetTick(); // 保存识别到 press 的 tick
            // me->debouncingTick = 0; // 清零去抖 tick，为 release 的去抖做准备
            InputEvent *evt = Q_NEW(InputEvent, SIGNAL_INPUT);
            evt->key = EC11_KEY_PRESS;
            QACTIVE_POST(g_InputProcess, &evt->super, &me->super);
            status =  Q_TRAN(Ec11ClickReleaseDebouncing);
        } else {
            status = Q_TRAN(Ec11Idle);
        }
        break;
    default:
        status = Q_SUPER(Ec11RotatePoll);
        break;
    }

    return status;
}

QState Ec11ClickReleaseDebouncing(Ec11 *me, const QEvt *e)
{
    QState status = Q_HANDLED();
    switch (e->sig)
    {
    case Q_ENTRY_SIG:
        QTimeEvt_armX(&me->timeEvtKeyPoll, EC11_KEY_POLL_INTERVAL, EC11_KEY_POLL_INTERVAL);
        status = Q_HANDLED();
        break;
    case Q_EXIT_SIG:
        QTimeEvt_disarm(&me->timeEvtKeyPoll);
        QTimeEvt_disarm(&me->timeEvtDebouncing);
        status = Q_HANDLED();
        break;
    case SIGNAL_KEY_POLL:
        if (gpio_get(EC11_KEY_PIN)) {
            QTimeEvt_armX(&me->timeEvtDebouncing, KEY_DEBOUNCING_TIME_10MS, 0); // 按下的过程中检测到高电平则启动释放的去抖
            QTimeEvt_disarm(&me->timeEvtKeyPoll);
        }
        status = Q_HANDLED();
        break;
    case SIGNAL_DEBOUNCING:
        if (gpio_get(EC11_KEY_PIN)) { // 释放按键去抖成功
            status = Q_TRAN(Ec11ClickRelease);
        } else {
            QTimeEvt_armX(&me->timeEvtKeyPoll, EC11_ROTATE_POLL_INTERVAL, EC11_ROTATE_POLL_INTERVAL); // 释放按键去抖失败则可能是干扰，继续轮询按键是否释放
            status = Q_HANDLED();
        }
        break;
    default:
        status = Q_SUPER(Ec11RotatePoll);
        break;
    }

    return status;
}

QState Ec11ClickRelease(Ec11 *me, const QEvt *e)
{
    QState status = Q_HANDLED();
    switch (e->sig)
    {
    case Q_ENTRY_SIG:
        if (me->hasRotate) {
            status = Q_TRAN(Ec11Idle);
            me->hasRotate = false;
            break;
        }
        uint32_t currentTick = Ec11GetTick();
        if (currentTick - me->lastPressTick <= KEY_LONG_CLICK_TIME_700MS) { // 单击
            me->lastClickTick = currentTick; // 更新单击 tick，判断连击的候使用
            me->lastPressTick = 0;
            me->clickCount++;
            status = Q_TRAN(Ec11ContinueClickPress);
        } else { // 长按
            InputEvent *inputEvt = Q_NEW(InputEvent, SIGNAL_INPUT);
            inputEvt->key = EC11_KEY_LONG_CLICK;
            QACTIVE_POST(g_InputProcess, &inputEvt->super, &me->super);
            status = Q_TRAN(Ec11Idle);
        }
        break;
    default:
        status = Q_SUPER(Ec11RotatePoll);
        break;
    }

    return status;
}

QState Ec11ContinueClickPress(Ec11 *me, const QEvt *e)
{
    QState status = Q_HANDLED();
    switch (e->sig)
    {
    case Q_ENTRY_SIG:
        QTimeEvt_armX(&me->timeEvtKeyPoll, EC11_ROTATE_POLL_INTERVAL, EC11_ROTATE_POLL_INTERVAL);
        status = Q_HANDLED();
        break;
    case Q_EXIT_SIG:
        QTimeEvt_disarm(&me->timeEvtKeyPoll);
        status = Q_HANDLED();
        break;
    case SIGNAL_KEY_POLL:
    {
        uint32_t currentTick = Ec11GetTick();
        uint32_t sinceLastClickDurationTick = currentTick - me->lastClickTick;
        bool currentKeyLevel = gpio_get(EC11_KEY_PIN);
        if (currentKeyLevel && sinceLastClickDurationTick > KEY_CONTINUE_TIME_200MS) {
            // 200ms的连击阈值内没有按下，则发送连击消息
            InputEvent *inputEvent = Q_NEW(InputEvent, SIGNAL_INPUT);
            inputEvent->key = EC11_KEY_CONTINUE_CLICK;
            inputEvent->data.kickCount = me->clickCount;
            QACTIVE_POST(g_InputProcess, &inputEvent->super, &me->super);
            status = Q_TRAN(Ec11Idle);
            break;
        }

        if (!currentKeyLevel && sinceLastClickDurationTick <= KEY_CONTINUE_TIME_200MS) {
            me->lastPressTick = currentTick;
            status = Q_TRAN(Ec11ContinueClickPressDebouncing);
            break;
        }
        status = Q_HANDLED();
        break;
    }
    default:
        status = Q_SUPER(Ec11RotatePoll);
        break;
    }

    return status;
}

QState Ec11ContinueClickPressDebouncing(Ec11 *me, const QEvt *e)
{
    QState status = Q_HANDLED();
    switch (e->sig)
    {
    case Q_ENTRY_SIG:
        QTimeEvt_armX(&me->timeEvtDebouncing, KEY_DEBOUNCING_TIME_10MS, 0); // 启动按下去抖定时器
        status = Q_HANDLED();
        break;
    case SIGNAL_DEBOUNCING:
        if (!gpio_get(EC11_KEY_PIN)) { // 按下去抖成功
            me->lastPressTick = Ec11GetTick();
            status = Q_TRAN(Ec11ContinueClickReleaseDebouncing);
        } else {
            status = Q_TRAN(Ec11ContinueClickPress); // 去抖失败则回退到 Ec11ContinueClickPress 继续轮询
        }
        break;
    default:
        status = Q_SUPER(Ec11RotatePoll);
        break;
    }

    return status;
}

QState Ec11ContinueClickReleaseDebouncing(Ec11 *me, const QEvt *e)
{
    QState status = Q_HANDLED();
    switch (e->sig)
    {
    case Q_ENTRY_SIG:
        QTimeEvt_armX(&me->timeEvtKeyPoll, EC11_ROTATE_POLL_INTERVAL, EC11_ROTATE_POLL_INTERVAL);
        status = Q_HANDLED();
        break;
    case Q_EXIT_SIG:
        QTimeEvt_disarm(&me->timeEvtKeyPoll);
        QTimeEvt_disarm(&me->timeEvtDebouncing);
        status = Q_HANDLED();
        break;
    case SIGNAL_KEY_POLL:
        if (gpio_get(EC11_KEY_PIN)) { // 按下的过程中检测到高电平则启动释放的去抖
            QTimeEvt_armX(&me->timeEvtDebouncing, KEY_DEBOUNCING_TIME_10MS, 0);
            QTimeEvt_disarm(&me->timeEvtKeyPoll);
        }
        status = Q_HANDLED();
        break;
    case SIGNAL_DEBOUNCING:
        if (gpio_get(EC11_KEY_PIN)) { // 释放按键去抖成功
            status = Q_TRAN(Ec11ContinueClickRelease);
        } else {
            QTimeEvt_armX(&me->timeEvtKeyPoll, EC11_ROTATE_POLL_INTERVAL, EC11_ROTATE_POLL_INTERVAL); // 去抖失败则在当前状态继续轮询
            status = Q_HANDLED();
        }
        break;
    default:
        status = Q_SUPER(Ec11RotatePoll);
        break;
    }

    return status;
}

QState Ec11ContinueClickRelease(Ec11 *me, const QEvt *e)
{
    QState status = Q_HANDLED();
    switch (e->sig)
    {
    case Q_ENTRY_SIG:
        if (me->hasRotate) { // 在按下的过程中旋转则直接转换到空闲状态
            status = Q_TRAN(Ec11Idle);
            me->hasRotate = false;
            break;
        }

        uint32_t currentTick = Ec11GetTick();
        if (currentTick - me->lastPressTick <= KEY_LONG_CLICK_TIME_700MS) { // 单击
            me->lastClickTick = currentTick; // 更新单击 tick，判断连击的候使用
            me->lastPressTick = 0;
            me->clickCount++;
            status = Q_TRAN(Ec11ContinueClickPress);
        } else { // 在连击的过程长按则退出
            status = Q_TRAN(Ec11Idle);
        }
        break;
    default:
        status = Q_SUPER(Ec11RotatePoll);
        break;
    }

    return status;
}
