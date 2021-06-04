
/*
void parsing_host_on_arrival(const unsigned state, struct selector_key *key) {
    proxyConnection *connection = ATTACHMENT(key);
    struct request_line_st *requestLine = &connection->client.request_line;

    requestLine->request_parser.request = &requestLine->request;
    requestLine->buffer = &connection->client_buffer;

    request_parser_init(&requestLine->request_parser);
}

void parsing_host_on_departure(const unsigned state, struct selector_key *key) {
    proxyConnection *connection = ATTACHMENT(key);
}

unsigned
parsing_host_on_read_ready(struct selector_key *key) {
    proxyConnection *connection = ATTACHMENT(key);

    struct request_line_st *requestLine = &connection->client.request_line;
    // Tengo que parsear la request del cliente para determinar a que host me conecto
    while (buffer_can_read(requestLine->buffer)) {
        uint8_t c = buffer_read(requestLine->buffer);
        putchar(c);  //FIXME: borrarlo
        request_state state = request_parser_feed(&requestLine->request_parser, c);
        if (state == request_error) {
            printf("BAD REQUEST\n");  //FIXME: DEVOLVER EN EL SOCKET AL CLIENTE BAD REQUEST
            break;
        } else if (state == request_done) {
            printf("REQUEST LINE PARSED!\nLine: %s\n", requestLine->request.method);
            return TRY_CONNECTION_IP;
            break;
        }
    }
    return connection->stm.current->state;
}*/
