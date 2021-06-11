#include "send_remaning_request.h"

void send_remaining_request_on_arrival(const unsigned state, struct selector_key *key);

unsigned
send_remaining_request_on_write_ready(struct selector_key *key);

#endif
