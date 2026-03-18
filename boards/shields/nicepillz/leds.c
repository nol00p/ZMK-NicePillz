#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/led.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zmk/ble.h>
#include <zmk/activity.h>
#include <zmk/events/ble_active_profile_changed.h>
#include <zmk/events/activity_state_changed.h>

#define LED_GPIO_NODE_ID DT_COMPAT_GET_ANY_STATUS_OKAY(gpio_leds)

#define LED_CAPS DT_NODE_CHILD_IDX(DT_ALIAS(led_caps))
#define LED_NUM  DT_NODE_CHILD_IDX(DT_ALIAS(led_num))

/* Idle blink timing */
#define IDLE_BLINK_ON_MS   1000
#define IDLE_BLINK_OFF_MS  2000

/* Pre-sleep flash pattern */
#define SLEEP_FLASH_COUNT    3
#define SLEEP_FLASH_ON_MS   80
#define SLEEP_FLASH_OFF_MS  80

static const struct device *led_dev = DEVICE_DT_GET(LED_GPIO_NODE_ID);
static bool idle_blink_state = false;
static bool is_idle = false;

/* --- Idle slow blink timer --- */

static void idle_blink_handler(struct k_timer *timer);
K_TIMER_DEFINE(idle_blink_timer, idle_blink_handler, NULL);

static void idle_blink_handler(struct k_timer *timer) {
    idle_blink_state = !idle_blink_state;
    if (idle_blink_state) {
        led_on(led_dev, LED_CAPS);
        k_timer_start(&idle_blink_timer, K_MSEC(IDLE_BLINK_ON_MS), K_NO_WAIT);
    } else {
        led_off(led_dev, LED_CAPS);
        k_timer_start(&idle_blink_timer, K_MSEC(IDLE_BLINK_OFF_MS), K_NO_WAIT);
    }
}

static void idle_blink_start(void) {
    is_idle = true;
    /* Turn off BLE LED during idle to save power */
    led_off(led_dev, LED_NUM);
    /* Start blink cycle: LED on first */
    idle_blink_state = true;
    led_on(led_dev, LED_CAPS);
    k_timer_start(&idle_blink_timer, K_MSEC(IDLE_BLINK_ON_MS), K_NO_WAIT);
}

static void idle_blink_stop(void) {
    is_idle = false;
    k_timer_stop(&idle_blink_timer);
}

/* --- Pre-sleep flash sequence (runs from work queue) --- */

static void sleep_flash_work_handler(struct k_work *work);
K_WORK_DEFINE(sleep_flash_work, sleep_flash_work_handler);

static void sleep_flash_work_handler(struct k_work *work) {
    /* Stop any idle blinking first */
    idle_blink_stop();
    led_off(led_dev, LED_CAPS);
    led_off(led_dev, LED_NUM);
    k_msleep(SLEEP_FLASH_OFF_MS);

    for (int i = 0; i < SLEEP_FLASH_COUNT; i++) {
        led_on(led_dev, LED_CAPS);
        k_msleep(SLEEP_FLASH_ON_MS);
        led_off(led_dev, LED_CAPS);
        if (i < SLEEP_FLASH_COUNT - 1) {
            k_msleep(SLEEP_FLASH_OFF_MS);
        }
    }

    /* LEDs off for deep sleep */
    led_off(led_dev, LED_CAPS);
    led_off(led_dev, LED_NUM);
}

/* --- Restore LEDs on wake --- */

static void leds_restore_active(void) {
    idle_blink_stop();
    /* Power LED back on */
    led_on(led_dev, LED_CAPS);
    /* Restore BLE LED state */
    if (zmk_ble_active_profile_is_connected()) {
        led_on(led_dev, LED_NUM);
    } else {
        led_off(led_dev, LED_NUM);
    }
}

/* --- Activity state listener --- */

static int activity_listener_cb(const zmk_event_t *eh) {
    enum zmk_activity_state state = zmk_activity_get_state();

    switch (state) {
    case ZMK_ACTIVITY_ACTIVE:
        leds_restore_active();
        break;
    case ZMK_ACTIVITY_IDLE:
        idle_blink_start();
        break;
    case ZMK_ACTIVITY_SLEEP:
        k_work_submit(&sleep_flash_work);
        break;
    }
    return 0;
}

ZMK_LISTENER(activity_led_listener, activity_listener_cb);
ZMK_SUBSCRIPTION(activity_led_listener, zmk_activity_state_changed);

/* --- BLE connection listener --- */

static int ble_listener_cb(const zmk_event_t *eh) {
    /* Only update BLE LED when not in idle/sleep */
    if (is_idle) {
        return 0;
    }
    if (zmk_ble_active_profile_is_connected()) {
        led_on(led_dev, LED_NUM);
    } else {
        led_off(led_dev, LED_NUM);
    }
    return 0;
}

ZMK_LISTENER(ble_led_listener, ble_listener_cb);
ZMK_SUBSCRIPTION(ble_led_listener, zmk_ble_active_profile_changed);

/* --- Init --- */

static int leds_init(const struct device *device) {
    if (!device_is_ready(led_dev)) {
        return -ENODEV;
    }

    /* Power ON - always on */
    led_on(led_dev, LED_CAPS);

    /* Set initial BLE state */
    if (zmk_ble_active_profile_is_connected()) {
        led_on(led_dev, LED_NUM);
    }

    return 0;
}

SYS_INIT(leds_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
