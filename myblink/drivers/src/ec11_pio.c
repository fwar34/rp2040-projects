#include "ec11.h"
#include "bsp.h"
#include "input_process.h"
#include "signals.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "pico/stdlib.h"
#include "irq_handler.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "logic.h"
#include "led.h"

#define EC11_KEY_PIN 23
#define EC11_A_PIN 28
#define EC11_B_PIN 29

// PIO 配置（固定使用PIO0，状态机0）
#define EC11_PIO_INST pio0
#define EC11_PIO_SM 0
// PIO 时钟分频（125MHz / 1 = 125MHz，最高精度）
#define EC11_PIO_CLKDIV 1.0f

// 按键相关宏（保留原有）
#define KEY_BUF_SIZE 4
#define KEY_DEBOUNCING_TIME_10MS 10
#define KEY_CONTINUE_TIME_200MS 200
#define KEY_LONG_CLICK_TIME_700MS 700
#define EC11_ROTATE_POLL_INTERVAL 1
#define EC11_KEY_POLL_INTERVAL 10

// 移除原有中断防抖宏，PIO 硬件防抖更可靠
#define ROTATE_DEBOUNCE_TIME_3MS 400

typedef struct {
    QActive super;
    uint32_t lastAIrqTime; // 保留但不再使用（兼容原有结构）
    uint32_t lastBIrqTime; // 保留但不再使用
    int32_t tmpCount;      // 保留，用于过渡计数
    uint8_t lastKeyLevel;
    uint32_t lastPressTick;
    uint8_t clickCount;
    uint32_t lastClickTick;
    int16_t rotatePosition; // EC11 旋转位置计数
    bool hasRotate;
    uint8_t currentKeyState;
    QTimeEvt timeEvtKeyPoll;
    QTimeEvt timeEvtDebouncing;
    QTimeEvt timeEvtRotatePoll;
    // PIO 新增变量：上一次读取的PIO计数，用于计算增量
    int32_t lastPioCount;
} Ec11;

static Ec11 g_Instance;
QActive *g_Ec11 = &g_Instance.super;

// ===================== PIO 程序定义 =====================
// PIO 程序：捕捉 EC11 A/B 相边沿，判断旋转方向并累计计数
// 工作原理：
// 1. 等待 A/B 任意一相电平变化
// 2. 读取当前 A/B 电平，判断旋转方向
// 3. 方向正确则 X 寄存器 +/-1，否则忽略（防抖）
// static const uint16_t ec11_pio_program_instructions[] = {
//     // 初始状态：等待 A/B 引脚电平变化
//     0x4080, // wait 0 pin 0          ; 等待 A 相（pin0）电平变化
//     0x40c0, // wait 0 pin 1          ; 等待 B 相（pin1）电平变化
//     // 读取当前 A/B 电平
//     0x4400, // in pins, 2            ; 读取 A(pin0)/B(pin1) 电平到 ISR (bit0=A, bit1=B)
//     0xa022, // mov y, isr            ; 保存当前电平到 Y 寄存器
//     // 判断旋转方向：
//     // 顺时针（右旋转）：A 先变，B 滞后 → 01 → 10
//     // 逆时针（左旋转）：B 先变，A 滞后 → 10 → 01
//     0x0042, // jmp y, 0x06           ; 如果 Y=00 或 11（无效状态），跳回等待
//     0x0082, // jmp y, 0x08           ; 如果 Y=01（右旋转），跳转到+1
//     0x00c2, // jmp y, 0x0a           ; 如果 Y=10（左旋转），跳转到-1
//     // 无效状态，跳回等待
//     0x0000, // jmp 0x00              ; 回到初始等待
//     // 右旋转：X += 1
//     0x0021, // inc x                 ; X 寄存器+1（右旋转计数）
//     0x0000, // jmp 0x00              ; 回到初始等待
//     // 左旋转：X -= 1
//     0x0020, // dec x                 ; X 寄存器-1（左旋转计数）
//     0x0000, // jmp 0x00              ; 回到初始等待
// };

// 修正后的PIO程序
// static const uint16_t ec11_pio_program_instructions[] = {
//     // 检查 A 或 B 引脚是否有边沿变化
//     0xa020, // mov x, pins           ; 将当前引脚状态保存到 X
//     0x4080, // wait 0 pin 0          ; 等待 A 相下降沿
//     0xa040, // mov y, pins           ; 读取当前 A/B 状态到 Y
//     0x0063, // jmp x, 2              ; 如果上一状态是 10 (0b10)，跳转到顺时针
//     0x00c5, // jmp x, 3              ; 如果上一状态是 11 (0b11)，跳转到顺时针
//     0x0027, // jmp x, 1              ; 如果上一状态是 01 (0b01)，跳转到逆时针
//     0x0000, // jmp 0                 ; 否则跳转到等待
//     // 顺时针旋转：X寄存器+1
//     0x0021, // inc x                 ; X+1
//     0xa0a1, // mov isr, x            ; 将X值移动到ISR
//     0xb800, // push block            ; 将ISR推入FIFO
//     0x0000, // jmp 0                 ; 跳回开始
//     // 逆时针旋转：X寄存器-1
//     0x0020, // dec x                 ; X-1
//     0xa0a1, // mov isr, x            ; 将X值移动到ISR
//     0xb800, // push block            ; 将ISR推入FIFO
//     0x0000, // jmp 0                 ; 跳回开始
// };

// 简化版本：使用GPIO电平变化检测
static const uint16_t ec11_pio_program_instructions[] = {
    // 读取当前A/B状态
    0x4400, // in pins, 2            ; 读取A/B引脚到ISR (0=GPIO0, 1=GPIO1)
    0xa022, // mov x, isr            ; 保存当前状态到X寄存器
    0x20c0, // pull block            ; 从FIFO获取上次状态到OSR
    0xa043, // mov y, osr            ; 保存上次状态到Y寄存器
    0x0048, // jmp x, 8              ; 如果当前状态为00，跳转到等待
    0x008a, // jmp x, 10             ; 如果当前状态为01，跳转到处理
    0x00cb, // jmp x, 11             ; 如果当前状态为10，跳转到处理
    0x000d, // jmp x, 13             ; 如果当前状态为11，跳转到处理
    
    // 状态00处理
    0x0000, // jmp 0                 ; 跳转到等待
    // 状态01处理
    0x0021, // inc x                 ; 顺时针计数+1
    0xa0a1, // mov isr, x            ; 移动到ISR
    0xb800, // push block            ; 推入FIFO
    0x0000, // jmp 0                 ; 跳转到等待
    // 状态10处理
    0x0020, // dec x                 ; 逆时针计数-1
    0xa0a1, // mov isr, x            ; 移动到ISR
    0xb800, // push block            ; 推入FIFO
    0x0000, // jmp 0                 ; 跳转到等待
    // 状态11处理
    0x0000, // jmp 0                 ; 跳转到等待
};

static const struct pio_program ec11_pio_program = {
    .instructions = ec11_pio_program_instructions,
    .length = count_of(ec11_pio_program_instructions),
    .origin = -1,
};

// ===================== 原有函数保留（按键相关） =====================
static uint32_t Ec11GetTick()
{
    return us_to_ms(time_us_32());
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

// ===================== PIO 初始化函数 =====================
static void Ec11PioInit(void)
{
    // 1. 加载 PIO 程序到 PIO 内存
    uint offset = pio_add_program(EC11_PIO_INST, &ec11_pio_program);
    
    // 2. 配置 PIO 状态机
    pio_sm_config sm_cfg = pio_get_default_sm_config();
    // 设置时钟分频
    sm_config_set_clkdiv(&sm_cfg, EC11_PIO_CLKDIV);
    // 设置引脚映射：SM 的 pin0 = EC11_A_PIN，pin1 = EC11_B_PIN
    sm_config_set_set_pins(&sm_cfg, EC11_A_PIN, 2);
    sm_config_set_in_pins(&sm_cfg, EC11_A_PIN);
    sm_config_set_sideset_pins(&sm_cfg, EC11_A_PIN);
    // 启用引脚输入
    sm_config_set_fifo_join(&sm_cfg, PIO_FIFO_JOIN_NONE);
    // 设置程序起始地址
    sm_config_set_wrap(&sm_cfg, offset, offset + count_of(ec11_pio_program_instructions) - 1);
    
    // 3. 初始化状态机
    pio_sm_init(EC11_PIO_INST, EC11_PIO_SM, offset, &sm_cfg);
    
    // 4. 配置 GPIO 为 PIO 控制
    pio_gpio_init(EC11_PIO_INST, EC11_A_PIN);
    pio_gpio_init(EC11_PIO_INST, EC11_B_PIN);
    gpio_pull_up(EC11_A_PIN);
    gpio_pull_up(EC11_B_PIN);
    
    // 5. 重置 X 寄存器（计数清零）
    pio_sm_exec(EC11_PIO_INST, EC11_PIO_SM, pio_encode_mov(pio_x, pio_null));
    g_Instance.lastPioCount = 0;
    
    // 6. 启用状态机
    pio_sm_set_enabled(EC11_PIO_INST, EC11_PIO_SM, true);
}

// ===================== 旋转事件上报函数（适配 PIO） =====================
static void PushEc11Event(int32_t delta)
{
    if (delta == 0) return;
    
    g_Instance.rotatePosition += delta;
    InputEvent *inputEvent = Q_NEW(InputEvent, SIGNAL_INPUT);
    inputEvent->data.rotatePosition = g_Instance.rotatePosition;
    
    if (delta > 0) {
        inputEvent->key = (g_Instance.lastPressTick != 0) ? EC11_KEY_PRESS_RIGHT_ROTATE : EC11_KEY_RIGHT_ROTATE;
    } else {
        inputEvent->key = (g_Instance.lastPressTick != 0) ? EC11_KEY_PRESS_LEFT_ROTATE : EC11_KEY_LEFT_ROTATE;
    }
    QACTIVE_POST(g_InputProcess, &inputEvent->super, &g_Instance.super);
}

// ===================== 原有状态机函数修改（替换旋转检测） =====================
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

void Ec11Ctor(void) 
{
    Ec11 *ec11 = &g_Instance;
    QActive_ctor(&ec11->super, Q_STATE_CAST(Ec11Init));
    QTimeEvt_ctorX(&ec11->timeEvtKeyPoll, &ec11->super, SIGNAL_KEY_POLL, 0U);
    QTimeEvt_ctorX(&ec11->timeEvtDebouncing, &ec11->super, SIGNAL_DEBOUNCING, 0U);
    QTimeEvt_ctorX(&ec11->timeEvtRotatePoll, &ec11->super, SIGNAL_ROTATE_POLL, 0U);
    
    // 初始化 PIO 计数
    ec11->lastPioCount = 0;
}

static void Ec11Reset(Ec11 *me)
{
    me->lastKeyLevel = 1;
    me->lastPressTick = 0;
    me->clickCount = 0;
    me->lastClickTick = 0;
    me->hasRotate = false;
}

static void LedTest(uint32_t now)
{
    static uint32_t ledLastTime = 0;
    Led0Xor();
    ledLastTime = now;
}

// ===================== 状态机核心函数修改 =====================
QState Ec11Init(Ec11 *me, const void *arg)
{
    Q_UNUSED_PAR(me);
    Q_UNUSED_PAR(arg);

    uint32_t now = time_us_32();
    me->lastAIrqTime = now;
    me->lastBIrqTime = now;
    me->rotatePosition = 0;
    
    // 初始化按键 GPIO（保留原有）
    gpio_init(EC11_KEY_PIN);
    gpio_set_dir(EC11_KEY_PIN, GPIO_IN);
    gpio_pull_up(EC11_KEY_PIN);
    
    // 初始化 PIO（替代原有 GPIO 中断）
    Ec11PioInit();
    
    // 启动旋转轮询定时器（读取 PIO 计数）
    QTimeEvt_armX(&me->timeEvtRotatePoll, EC11_ROTATE_POLL_INTERVAL, EC11_ROTATE_POLL_INTERVAL);
    
    return Q_TRAN(Ec11Idle);
}

// 核心修改：旋转轮询状态，读取 PIO 计数并生成事件
QState Ec11RotatePoll(Ec11 *me, const QEvt *e)
{
    QState status = Q_HANDLED();
    switch (e->sig)
    {
    case SIGNAL_ROTATE_POLL:
    {
        Logic2Xor();
        static uint32_t count = 0;
        if (count++ % 500 == 0) {
            InputEvent *inputEvent = Q_NEW(InputEvent, SIGNAL_INPUT);
            inputEvent->data.qeueueMin = QActive_getQueueMin(me->super.prio);
            inputEvent->key = EC11_DEBUG;
            QACTIVE_POST(g_InputProcess, &inputEvent->super, &me->super);
        }

        if (count % 1000 == 0) {
            // 打印 PIO 计数（替代原有中断计数）
            int32_t currentPioCount = pio_sm_get(EC11_PIO_INST, EC11_PIO_SM);
            printf("PIO Count=%d, RotatePosition=%d\n", currentPioCount, me->rotatePosition);
        }

        // 读取 PIO X 寄存器的累计计数
        int32_t currentPioCount = pio_sm_get(EC11_PIO_INST, EC11_PIO_SM);
        // 计算与上一次的差值
        int32_t delta = currentPioCount - me->lastPioCount;
        
        // 累计差值达到 4 则上报事件（兼容原有逻辑）
        if (abs(delta) >= 4) {
            // PushEc11Event(delta);
            // 重置 PIO X 寄存器和本地计数
            pio_sm_exec(EC11_PIO_INST, EC11_PIO_SM, pio_encode_mov(pio_x, pio_null));
            me->lastPioCount = 0;
        } else if (delta != 0) {
            // 未达到阈值，保存当前计数
            me->lastPioCount = currentPioCount;
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

// 以下按键相关状态机函数完全保留原有逻辑，无需修改
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
        status = Q_SUPER(Ec11RotatePoll);
        break;
    }

    return status;
}

QState Ec11ClickPressDebouncing(Ec11 *me, const QEvt *e)
{
    QState status = Q_HANDLED();
    switch (e->sig)
    {
    case Q_ENTRY_SIG:
        if (gpio_get(EC11_KEY_PIN)) {
            status = Q_TRAN(Ec11Idle);
        } else {
            QTimeEvt_armX(&me->timeEvtDebouncing, KEY_DEBOUNCING_TIME_10MS, 0U);
            status = Q_HANDLED();
        }
        break;
    case SIGNAL_DEBOUNCING:
        if (!gpio_get(EC11_KEY_PIN)) {
            me->lastPressTick = Ec11GetTick();
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
            QTimeEvt_armX(&me->timeEvtDebouncing, KEY_DEBOUNCING_TIME_10MS, 0);
            QTimeEvt_disarm(&me->timeEvtKeyPoll);
        }
        status = Q_HANDLED();
        break;
    case SIGNAL_DEBOUNCING:
        if (gpio_get(EC11_KEY_PIN)) {
            status = Q_TRAN(Ec11ClickRelease);
        } else {
            QTimeEvt_armX(&me->timeEvtKeyPoll, EC11_ROTATE_POLL_INTERVAL, EC11_ROTATE_POLL_INTERVAL);
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
        if (currentTick - me->lastPressTick <= KEY_LONG_CLICK_TIME_700MS) {
            me->lastClickTick = currentTick;
            me->lastPressTick = 0;
            me->clickCount++;
            status = Q_TRAN(Ec11ContinueClickPress);
        } else {
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
        QTimeEvt_armX(&me->timeEvtDebouncing, KEY_DEBOUNCING_TIME_10MS, 0);
        status = Q_HANDLED();
        break;
    case SIGNAL_DEBOUNCING:
        if (!gpio_get(EC11_KEY_PIN)) {
            me->lastPressTick = Ec11GetTick();
            status = Q_TRAN(Ec11ContinueClickReleaseDebouncing);
        } else {
            status = Q_TRAN(Ec11ContinueClickPress);
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
        if (gpio_get(EC11_KEY_PIN)) {
            QTimeEvt_armX(&me->timeEvtDebouncing, KEY_DEBOUNCING_TIME_10MS, 0);
            QTimeEvt_disarm(&me->timeEvtKeyPoll);
        }
        status = Q_HANDLED();
        break;
    case SIGNAL_DEBOUNCING:
        if (gpio_get(EC11_KEY_PIN)) {
            status = Q_TRAN(Ec11ContinueClickRelease);
        } else {
            QTimeEvt_armX(&me->timeEvtKeyPoll, EC11_ROTATE_POLL_INTERVAL, EC11_ROTATE_POLL_INTERVAL);
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
        if (me->hasRotate) {
            status = Q_TRAN(Ec11Idle);
            me->hasRotate = false;
            break;
        }

        uint32_t currentTick = Ec11GetTick();
        if (currentTick - me->lastPressTick <= KEY_LONG_CLICK_TIME_700MS) {
            me->lastClickTick = currentTick;
            me->lastPressTick = 0;
            me->clickCount++;
            status = Q_TRAN(Ec11ContinueClickPress);
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