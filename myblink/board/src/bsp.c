#include "bsp.h"    // Board Support Package
// #include "blinky.h" // Blinky Application interface
// #include "main.h"
#include "dht11.h"
#include "ec11.h"
#include "input_process.h"
// #include "signals.h"
#include "uart1.h"
// #include "st7735s.h"
#include "led.h"
#include "logic.h"
#include "pico/stdlib.h"
#include "pico/time.h"
#include <stdio.h>

// Q_DEFINE_THIS_FILE // define the name of this file for assertions

// Local-scope objects -----------------------------------------------------
//============================================================================
// Error handler and ISRs...

Q_NORETURN Q_onError(char const *const module, int_t const id)
{
    // NOTE: this implementation of the error handler is intended only
    // for debugging and MUST be changed for deployment of the application
    // (assuming that you ship your production code with assertions enabled).
    Q_UNUSED_PAR(module);
    Q_UNUSED_PAR(id);
    QS_ASSERTION(module, id, (uint32_t)10000U);
    printf("error:m:%s, id:%d\n", module, id);

    for (;;) { // for debugging, hang on in an endless loop...
        LedForError();
    }
}
//............................................................................
void assert_failed(char const *const module, int_t const id); // prototype
void assert_failed(char const *const module, int_t const id)
{
    Q_onError(module, id);
}
//............................................................................
void setup_qpc_timer(void);
void BSP_init(void)
{
    setup_qpc_timer();
}
void BSP_start(void)
{
    // initialize event pools
    static QF_MPOOL_EL(InputEvent) smlPoolSto[200];
    QF_poolInit(smlPoolSto, sizeof(smlPoolSto), sizeof(smlPoolSto[0]));

    // initialize publish-subscribe
    static QSubscrList subscrSto[PUB_SIGNAL_MAX];
    QActive_psInit(subscrSto, Q_DIM(subscrSto));

    // instantiate and start AOs/threads...

    static QEvtPtr dht11QueueSto[20];
    Dht11Ctor();
    QActive_start(g_Dht11,
                  1U,                    // QP prio. of the AO
                  dht11QueueSto,        // event queue storage
                  Q_DIM(dht11QueueSto), // queue length [events]
                  (void *)0, 0U,         // no stack storage
                  (void *)0);            // no initialization param

    static QEvtPtr ec11QueueSto[20];
    Ec11Ctor();
    QActive_start(g_Ec11,
                  3U,                    // QP prio. of the AO
                  ec11QueueSto,        // event queue storage
                  Q_DIM(ec11QueueSto), // queue length [events]
                  (void *)0, 0U,         // no stack storage
                  (void *)0);            // no initialization param

    static QEvtPtr inputProcessQueueSto[30];
    InputProcessCtor();
    QActive_start(g_InputProcess,
                  4U,                    // QP prio. of the AO
                  inputProcessQueueSto,        // event queue storage
                  Q_DIM(inputProcessQueueSto), // queue length [events]
                  (void *)0, 0U,         // no stack storage
                  (void *)0);            // no initialization param

    static QEvtPtr uartProcessQueueSto[250];
    UartProcessCtor();
    QActive_start(g_UartProcess,
                  2U,                    // QP prio. of the AO
                  uartProcessQueueSto,        // event queue storage
                  Q_DIM(uartProcessQueueSto), // queue length [events]
                  (void *)0, 0U,         // no stack storage
                  (void *)0);            // no initialization param
}
//............................................................................
void BSP_ledOn(void)
{
    // LightLed1();
}
//............................................................................
void BSP_ledOff(void)
{
    // OffLed1();
}
//............................................................................
void BSP_terminate(int16_t result) { Q_UNUSED_PAR(result); }

//============================================================================
// QF callbacks...
void QF_onStartup(void) {}
//............................................................................
void QF_onCleanup(void) {}
//............................................................................
void QV_onIdle(void)
{ // CATION: called with interrupts DISABLED, NOTE2
    Led1Xor();

    QF_INT_ENABLE(); // just enable interrupts
}

int64_t qpc_tick_callback(alarm_id_t id, void *user_data)
{
#if (defined LOGIC_TEST)
    // LogicUp();
#endif
    QTIMEEVT_TICK_X(0U, NULL);
#if (defined LOGIC_TEST)
    // LogicDown();
#endif
    return 1000; // 每毫秒重复一次
}

// 在初始化时调用
void setup_qpc_timer(void)
{
    add_alarm_in_us(1000, qpc_tick_callback, NULL, true);
}