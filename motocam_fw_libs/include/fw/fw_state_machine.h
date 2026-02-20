#ifndef FW_STATE_MACHINE_H
#define FW_STATE_MACHINE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <time.h>

/* ------------------------------------------------------------------ */
/*  Generic state-machine status returned by every _step() function   */
/* ------------------------------------------------------------------ */
typedef enum {
  FW_SM_IDLE,     /* not yet started                           */
  FW_SM_RUNNING,  /* still in progress – call _step() again    */
  FW_SM_DONE_OK,  /* completed successfully                    */
  FW_SM_DONE_FAIL /* completed with failure                    */
} fw_sm_status_t;

/* ------------------------------------------------------------------ */
/*  Monotonic timer – wraps clock_gettime(CLOCK_MONOTONIC)            */
/* ------------------------------------------------------------------ */
typedef struct {
  struct timespec start;
  uint32_t target_ms; /* how long to wait (milliseconds)  */
} fw_sm_timer_t;

/** Record current time and set the target delay. */
void fw_sm_timer_start(fw_sm_timer_t *t, uint32_t delay_ms);

/** Return 1 when the target delay has elapsed, 0 otherwise. */
int fw_sm_timer_expired(const fw_sm_timer_t *t);

/* ------------------------------------------------------------------ */
/*  Non-blocking yield – 10 ms usleep to avoid busy-spin              */
/* ------------------------------------------------------------------ */
void fw_sm_yield(void);

/* ------------------------------------------------------------------ */
/*  ONVIF interface switch state machine                              */
/* ------------------------------------------------------------------ */
typedef enum {
  ONVIF_SM_IDLE,
  ONVIF_SM_WAIT_STOP,  /* waiting after killall               */
  ONVIF_SM_START_CMD,  /* launch the update script            */
  ONVIF_SM_POLL_START, /* polling is_running() with retries   */
  ONVIF_SM_DONE_OK,
  ONVIF_SM_DONE_FAIL
} onvif_sm_state_t;

typedef struct {
  onvif_sm_state_t state;
  fw_sm_timer_t timer;
  uint8_t interface; /* 0 = eth, 1 = wifi                */
  uint8_t retry_count;
  uint8_t max_retries;
} onvif_sm_ctx_t;

void onvif_sm_init(onvif_sm_ctx_t *ctx, uint8_t interface);
fw_sm_status_t onvif_sm_step(onvif_sm_ctx_t *ctx);

/* ------------------------------------------------------------------ */
/*  WiFi status polling state machine                                 */
/* ------------------------------------------------------------------ */
typedef enum {
  WIFI_SM_IDLE,
  WIFI_SM_POLLING,
  WIFI_SM_DONE_OK,
  WIFI_SM_DONE_FAIL
} wifi_sm_state_t;

typedef struct {
  wifi_sm_state_t state;
  fw_sm_timer_t timer;
  int retry_count;
  int max_retries;
} wifi_sm_ctx_t;

void wifi_sm_init(wifi_sm_ctx_t *ctx, int max_retries);
fw_sm_status_t wifi_sm_step(wifi_sm_ctx_t *ctx);

/* ------------------------------------------------------------------ */
/*  Generic process-start polling state machine                       */
/* ------------------------------------------------------------------ */
typedef enum {
  PROC_SM_IDLE,
  PROC_SM_POLLING,
  PROC_SM_DONE_OK,
  PROC_SM_DONE_FAIL
} proc_sm_state_t;

typedef struct {
  proc_sm_state_t state;
  fw_sm_timer_t timer;
  const char *process_name;
  int retry_count;
  int max_retries;
} proc_sm_ctx_t;

void proc_sm_init(proc_sm_ctx_t *ctx, const char *process_name,
                  int max_retries);
fw_sm_status_t proc_sm_step(proc_sm_ctx_t *ctx);

/* ------------------------------------------------------------------ */
/*  Shutdown / OTA delay state machine                                */
/* ------------------------------------------------------------------ */
typedef enum {
  DELAY_SM_IDLE,
  DELAY_SM_WAITING,
  DELAY_SM_DONE
} delay_sm_state_t;

typedef struct {
  delay_sm_state_t state;
  fw_sm_timer_t timer;
  uint32_t delay_ms;
} delay_sm_ctx_t;

void delay_sm_init(delay_sm_ctx_t *ctx, uint32_t delay_ms);
fw_sm_status_t delay_sm_step(delay_sm_ctx_t *ctx);

/* ------------------------------------------------------------------ */
/*  Streamer restart (set_misc) state machine                         */
/* ------------------------------------------------------------------ */
typedef enum {
  MISC_SM_IDLE,
  MISC_SM_WAIT_STOP,    /* 1 s delay after stop_process()      */
  MISC_SM_POLL_RESTART, /* poll is_running()                    */
  MISC_SM_DONE
} misc_sm_state_t;

typedef struct {
  misc_sm_state_t state;
  fw_sm_timer_t timer;
} misc_sm_ctx_t;

void misc_sm_init(misc_sm_ctx_t *ctx);
fw_sm_status_t misc_sm_step(misc_sm_ctx_t *ctx);

#ifdef __cplusplus
}
#endif

#endif /* FW_STATE_MACHINE_H */
