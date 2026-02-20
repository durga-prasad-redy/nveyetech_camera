#include "fw/fw_state_machine.h"
#include "fw.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/* ================================================================== */
/*  Timer helpers                                                     */
/* ================================================================== */

void fw_sm_timer_start(fw_sm_timer_t *t, uint32_t delay_ms) {
  clock_gettime(CLOCK_MONOTONIC, &t->start);
  t->target_ms = delay_ms;
}

int fw_sm_timer_expired(const fw_sm_timer_t *t) {
  struct timespec now;
  clock_gettime(CLOCK_MONOTONIC, &now);

  long elapsed_ms = (now.tv_sec - t->start.tv_sec) * 1000L +
                    (now.tv_nsec - t->start.tv_nsec) / 1000000L;

  return elapsed_ms >= (long)t->target_ms;
}

void fw_sm_yield(void) {
  usleep(10000); /* 10 ms – yield CPU without hard-blocking */
}

/* ================================================================== */
/*  Generic delay state machine                                       */
/* ================================================================== */

void delay_sm_init(delay_sm_ctx_t *ctx, uint32_t delay_ms) {
  ctx->state = DELAY_SM_IDLE;
  ctx->delay_ms = delay_ms;
}

fw_sm_status_t delay_sm_step(delay_sm_ctx_t *ctx) {
  switch (ctx->state) {
  case DELAY_SM_IDLE:
    fw_sm_timer_start(&ctx->timer, ctx->delay_ms);
    ctx->state = DELAY_SM_WAITING;
    return FW_SM_RUNNING;

  case DELAY_SM_WAITING:
    if (fw_sm_timer_expired(&ctx->timer)) {
      ctx->state = DELAY_SM_DONE;
      return FW_SM_DONE_OK;
    }
    return FW_SM_RUNNING;

  case DELAY_SM_DONE:
    return FW_SM_DONE_OK;

  default:
    return FW_SM_DONE_FAIL;
  }
}

/* ================================================================== */
/*  Generic process-start polling state machine                       */
/* ================================================================== */

void proc_sm_init(proc_sm_ctx_t *ctx, const char *process_name,
                  int max_retries) {
  ctx->state = PROC_SM_IDLE;
  ctx->process_name = process_name;
  ctx->retry_count = 0;
  ctx->max_retries = max_retries;
}

fw_sm_status_t proc_sm_step(proc_sm_ctx_t *ctx) {
  switch (ctx->state) {
  case PROC_SM_IDLE:
    /* Already running? */
    if (is_running(ctx->process_name)) {
      ctx->state = PROC_SM_DONE_OK;
      return FW_SM_DONE_OK;
    }
    fw_sm_timer_start(&ctx->timer, 1000);
    ctx->state = PROC_SM_POLLING;
    return FW_SM_RUNNING;

  case PROC_SM_POLLING:
    if (!fw_sm_timer_expired(&ctx->timer))
      return FW_SM_RUNNING;

    /* Timer expired – check process */
    if (is_running(ctx->process_name)) {
      printf("[INFO] process :%s started successfully.\n", ctx->process_name);
      ctx->state = PROC_SM_DONE_OK;
      return FW_SM_DONE_OK;
    }

    ctx->retry_count++;
    printf("[DEBUG] Attempt %d...\n", ctx->retry_count);

    if (ctx->retry_count >= ctx->max_retries) {
      printf("[ERROR] Failed to start process :%s after all retries.\n",
             ctx->process_name);
      ctx->state = PROC_SM_DONE_FAIL;
      return FW_SM_DONE_FAIL;
    }

    fw_sm_timer_start(&ctx->timer, 1000);
    return FW_SM_RUNNING;

  case PROC_SM_DONE_OK:
    return FW_SM_DONE_OK;
  case PROC_SM_DONE_FAIL:
    return FW_SM_DONE_FAIL;

  default:
    return FW_SM_DONE_FAIL;
  }
}

/* ================================================================== */
/*  WiFi status polling state machine                                 */
/* ================================================================== */

/* Forward-declare the per-file helper (defined in fw_network.c) */
extern int read_wifi_status_once(void);

void wifi_sm_init(wifi_sm_ctx_t *ctx, int max_retries) {
  ctx->state = WIFI_SM_IDLE;
  ctx->retry_count = 0;
  ctx->max_retries = max_retries;
}

fw_sm_status_t wifi_sm_step(wifi_sm_ctx_t *ctx) {
  switch (ctx->state) {
  case WIFI_SM_IDLE:
    printf("[INFO] Checking Wi-Fi status with %d retries...\n",
           ctx->max_retries);
    /* Check immediately before waiting */
    if (read_wifi_status_once()) {
      ctx->state = WIFI_SM_DONE_OK;
      return FW_SM_DONE_OK;
    }
    ctx->retry_count = 1;
    fw_sm_timer_start(&ctx->timer, 1000);
    ctx->state = WIFI_SM_POLLING;
    return FW_SM_RUNNING;

  case WIFI_SM_POLLING:
    if (!fw_sm_timer_expired(&ctx->timer))
      return FW_SM_RUNNING;

    printf("[DEBUG] Attempt %d...\n", ctx->retry_count);

    if (read_wifi_status_once()) {
      ctx->state = WIFI_SM_DONE_OK;
      return FW_SM_DONE_OK;
    }

    ctx->retry_count++;
    if (ctx->retry_count > ctx->max_retries) {
      printf("[ERROR] Wi-Fi status check failed after all retries.\n");
      ctx->state = WIFI_SM_DONE_FAIL;
      return FW_SM_DONE_FAIL;
    }

    fw_sm_timer_start(&ctx->timer, 1000);
    return FW_SM_RUNNING;

  case WIFI_SM_DONE_OK:
    return FW_SM_DONE_OK;
  case WIFI_SM_DONE_FAIL:
    return FW_SM_DONE_FAIL;

  default:
    return FW_SM_DONE_FAIL;
  }
}

/* ================================================================== */
/*  ONVIF interface-switch state machine                              */
/* ================================================================== */

/* Commands needed by the ONVIF SM (mirrored from fw_network.c) */
#define SM_SET_ONVIF_INTERFACE_ETHERNET                                        \
  "/mnt/flash/vienna/scripts/onvif/onvif_update.sh eth0"
#define SM_SET_ONVIF_INTERFACE_WIFI                                            \
  "/mnt/flash/vienna/scripts/onvif/onvif_update.sh wlan0"

void onvif_sm_init(onvif_sm_ctx_t *ctx, uint8_t interface) {
  memset(ctx, 0, sizeof(*ctx));
  ctx->state = ONVIF_SM_IDLE;
  ctx->interface = interface;
  ctx->max_retries = 10;
}

fw_sm_status_t onvif_sm_step(onvif_sm_ctx_t *ctx) {
  switch (ctx->state) {
  case ONVIF_SM_IDLE:
    /* If ONVIF is running, stop it and wait 3 s */
    if (is_running(ONVIF_SERVER_PROCESS_NAME)) {
      stop_process(ONVIF_SERVER_PROCESS_NAME);
      printf("stopped onvif server process before changing interface\n");
      fw_sm_timer_start(&ctx->timer, 3000);
      ctx->state = ONVIF_SM_WAIT_STOP;
    } else {
      ctx->state = ONVIF_SM_START_CMD;
    }
    return FW_SM_RUNNING;

  case ONVIF_SM_WAIT_STOP:
    if (!fw_sm_timer_expired(&ctx->timer))
      return FW_SM_RUNNING;
    ctx->state = ONVIF_SM_START_CMD;
    return FW_SM_RUNNING;

  case ONVIF_SM_START_CMD: {
    /* Set env + launch the background script */
    if (ctx->interface == 0) {
      set_uboot_env_chars("onvif_itrf", "eth");
      char bg[600];
      snprintf(bg, sizeof(bg), "%s < /dev/null > /dev/null 2>&1 &",
               SM_SET_ONVIF_INTERFACE_ETHERNET);
      system(bg);
    } else {
      set_uboot_env_chars("onvif_itrf", "wifi");
      char bg[600];
      snprintf(bg, sizeof(bg), "%s < /dev/null > /dev/null 2>&1 &",
               SM_SET_ONVIF_INTERFACE_WIFI);
      system(bg);
    }
    ctx->retry_count = 0;
    fw_sm_timer_start(&ctx->timer, 1000);
    ctx->state = ONVIF_SM_POLL_START;
    return FW_SM_RUNNING;
  }

  case ONVIF_SM_POLL_START:
    if (!fw_sm_timer_expired(&ctx->timer))
      return FW_SM_RUNNING;

    if (is_running(ONVIF_SERVER_PROCESS_NAME)) {
      ctx->state = ONVIF_SM_DONE_OK;
      return FW_SM_DONE_OK;
    }

    ctx->retry_count++;
    printf("waiting for onvif server to start with count:%d\n",
           ctx->retry_count);

    if (ctx->retry_count > ctx->max_retries) {
      LOG_ERROR("NOT ABLE TO RESTART ONVIF SERVER");
      ctx->state = ONVIF_SM_DONE_FAIL;
      return FW_SM_DONE_FAIL;
    }

    fw_sm_timer_start(&ctx->timer, 1000);
    return FW_SM_RUNNING;

  case ONVIF_SM_DONE_OK:
    return FW_SM_DONE_OK;
  case ONVIF_SM_DONE_FAIL:
    return FW_SM_DONE_FAIL;

  default:
    return FW_SM_DONE_FAIL;
  }
}

/* ================================================================== */
/*  Streamer restart (set_misc) state machine                         */
/* ================================================================== */

void misc_sm_init(misc_sm_ctx_t *ctx) { ctx->state = MISC_SM_IDLE; }

fw_sm_status_t misc_sm_step(misc_sm_ctx_t *ctx) {
  switch (ctx->state) {
  case MISC_SM_IDLE:
    fw_sm_timer_start(&ctx->timer, 1000);
    ctx->state = MISC_SM_WAIT_STOP;
    return FW_SM_RUNNING;

  case MISC_SM_WAIT_STOP:
    if (!fw_sm_timer_expired(&ctx->timer))
      return FW_SM_RUNNING;

    ctx->state = MISC_SM_POLL_RESTART;
    return FW_SM_RUNNING;

  case MISC_SM_POLL_RESTART:
    if (is_running(STREAMER_PROCESS_NAME)) {
      ctx->state = MISC_SM_DONE;
      return FW_SM_DONE_OK;
    }
    /* Yield and retry on next step */
    return FW_SM_RUNNING;

  case MISC_SM_DONE:
    return FW_SM_DONE_OK;

  default:
    return FW_SM_DONE_FAIL;
  }
}
