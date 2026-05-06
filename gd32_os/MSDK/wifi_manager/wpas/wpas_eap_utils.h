
/**
 * WPAS_SM_ENTRY - State machine function entry point
 * @machine: State machine name
 * @state: State machine state
 *
 * This macro is used inside each state machine function declared with
 * SM_STATE. WPAS_SM_ENTRY should be in the beginning of the function body, but
 * after declaration of possible local variables. This macro prints debug
 * information about state transition and update the state machine state.
 */
#define WPAS_SM_ENTRY(machine, state) \
if (!global || sm->machine ## _state != machine ## _ ## state) { \
    sm->changed = true; \
    wpa_printf(STATE_MACHINE_DEBUG_PREFIX ": " #machine \
            " entering state " #state "\r\n"); \
} \
sm->machine ## _state = machine ## _ ## state;

char * base64_encode(const void *src, size_t len, size_t *out_len);
unsigned char * base64_decode(const char *src, size_t len, size_t *out_len);
