#include "request_line_parser.h"

#include <arpa/inet.h>
#include <stdbool.h>
#include <string.h>

#include "utils/parser/parser_utils.h"

static enum request_state
r_method(uint8_t c, struct request_parser *p);

static enum request_state
r_target_scheme(const uint8_t c, struct request_parser *p);

static enum request_state
r_target_host(const uint8_t c, struct request_parser *p);

static enum request_state
getMethodState(struct request_parser *p);

static enum request_state
r_target_port(const uint8_t c, struct request_parser *p);

static enum request_state
r_target_ogform(const uint8_t c, struct request_parser *p);

static enum request_state
r_version(const uint8_t c, struct request_parser *p);

static enum request_state
r_version_major(const uint8_t c, struct request_parser *p);

static enum request_state
r_version_minor(const uint8_t c, struct request_parser *p);

static enum request_state
r_end(const uint8_t c, struct request_parser *p);

static bool
parseIpv6(struct request_parser *p);

static bool
parseIpv4(struct request_parser *p);

static void assign_port(struct request_parser *p);

void request_parser_init(struct request_parser *p) {
    p->state = request_method;
    p->i = 0;
    p->n = MAX_METHOD_LENGTH;
    memset(p->request, 0, sizeof(*(p->request)));
}

enum request_state request_parser_feed(struct request_parser *p, const uint8_t c) {
    enum request_state next;

    switch (p->state) {
        case request_method:
            next = r_method(c, p);
            break;
        case request_target_scheme:
            next = r_target_scheme(c, p);
            break;
        case request_target_host:
            next = r_target_host(c, p);
            break;
        case request_target_port:
            next = r_target_port(c, p);
            break;
        case request_target_ogform:
            next = r_target_ogform(c, p);
            break;
        case request_version:
            next = r_version(c, p);
            break;
        case request_version_major:
            next = r_version_major(c, p);
            break;
        case request_version_minor:
            next = r_version_minor(c, p);
            break;
        case request_end:
            next = r_end(c, p);
            break;
        case request_done:
        case request_error:
        case request_error_unsupported_method:
        case request_error_unsupported_version:
            next = p->state;
            break;
        default:
            next = request_error;
            break;
    }

    return p->state = next;
}

static enum request_state
r_method(const uint8_t c, struct request_parser *p) {
    if (p->i < p->n) {
        if (IS_TOKEN(c)) {
            p->request->method[p->i++] = c;
            return request_method;
        } else if (c == ' ') {
            p->request->method[p->i] = 0;
            p->i = 0;
            return getMethodState(p);
        }
    }
    return request_error_unsupported_method;
}

static enum request_state
getMethodState(struct request_parser *p) {
    char *method = (char *) p->request->method;

    if (strcmp(method, CONNECT) != 0) { //FIXME: 
        p->request->request_target.type = absolute_form;
        p->n = SCHEME_LENGTH;
        return request_target_scheme;
    }

    p->n = MAX_FQDN_LENGTH;
    p->request->request_target.type = authority_form;
    return request_target_host;
}

static enum request_state
r_target_scheme(const uint8_t c, struct request_parser *p) {
    char *scheme = (char *) SCHEME;

    if (p->i >= p->n || scheme[p->i] != c)
        return request_error;

    if (p->i == p->n - 1) {
        p->i = 0;
        p->n = MAX_FQDN_LENGTH;
        return request_target_host;
    }

    p->i++;
    return request_target_scheme;
}

/*   
    The host subcomponent of authority is identified by an IP literal
    encapsulated within square brackets, an IPv4 address in dotted-
    decimal form, or a registered name.

    The authority component is preceded by a double slash ("//") and is
    terminated by the next slash ("/"), question mark ("?"), or number
    sign ("#") character, or by the end of the URI.
*/
static enum request_state
r_target_host(const uint8_t c, struct request_parser *p) {
    if (p->i >= p->n)
        return request_error;

    if (p->i == 0 && c == '[') {
        p->request->request_target.host_type = ipv6;
        return request_target_host;
    }
    // http://[1fff:0:a88:85a3::ac1f]/index.html

    if (p->request->request_target.host_type == ipv6) {
        if (c == ']') {
            p->request->request_target.host.domain[p->i++] = c;  //lo copio y despues lo piso
            return request_target_host;
        }

        if (c == ':' && p->request->request_target.host.domain[p->i - 1] == ']') {
            p->request->request_target.host.domain[p->i - 1] = 0;
            if (!parseIpv6(p))
                return request_error;
            p->i = 0;
            p->n = MAX_PORT_LENGTH;
            return request_target_port;
        }

        if (END_OF_AUTHORITY(c)) {
            p->request->request_target.host.domain[p->i - 1] = 0;
            if (!parseIpv6(p))
                return request_error;
            p->i = 0;
            p->n = MAX_ORIGIN_FORM;
            p->request->request_target.origin_form[p->i] = c;
            return request_target_ogform;
        }

        if (c == ' ') {
            p->request->request_target.host.domain[p->i - 1] = 0;
            if (!parseIpv6(p))
                return request_error;
            p->request->request_target.origin_form[0] = '/';
            p->i = 0;
            p->n = VERSION_LENGTH;
            return request_version;
        }
        p->request->request_target.host.domain[p->i++] = c;
        return request_target_host;
    }

    if (c == ':') {
        parseIpv4(p);
        p->i = 0;
        p->n = MAX_PORT_LENGTH;
        return request_target_port;
    }

    if (END_OF_AUTHORITY(c)) {
        parseIpv4(p);
        p->i = 0;
        p->n = MAX_ORIGIN_FORM;
        p->request->request_target.origin_form[p->i] = c;
        return request_target_ogform;
    }

    if (c == ' ') {
        parseIpv4(p);
        p->request->request_target.origin_form[0] = '/';
        p->i = 0;
        p->n = VERSION_LENGTH;
        return request_version;
    }

    p->request->request_target.host.domain[p->i++] = c;

    return request_target_host;
}

static bool parseIpv6(struct request_parser *p) {
    struct sockaddr_in6 ipv6addr;
    int addrlen = sizeof(ipv6addr);

    memset(&ipv6addr, 0, addrlen);

    if (inet_pton(AF_INET6, p->request->request_target.host.domain, &ipv6addr.sin6_addr) <= 0) {
        return false;
    }

    ipv6addr.sin6_family = AF_INET6;
    ipv6addr.sin6_port = htons(80);
    p->request->request_target.port = ipv6addr.sin6_port;

    p->request->request_target.host.ipv6 = ipv6addr;
    p->request->request_target.host_type = ipv6;

    return true;
}

static bool parseIpv4(struct request_parser *p) {
    struct sockaddr_in ipv4addr;
    int addrlen = sizeof(ipv4addr);

    memset(&ipv4addr, 0, addrlen);

    if (inet_pton(AF_INET, p->request->request_target.host.domain, &ipv4addr.sin_addr) <= 0) {
        p->request->request_target.port = htons(80);
        p->request->request_target.host_type = domain;
        return false;
    }

    ipv4addr.sin_family = AF_INET;
    ipv4addr.sin_port = htons(80);
    p->request->request_target.port = ipv4addr.sin_port;
    p->request->request_target.host.ipv4 = ipv4addr;
    p->request->request_target.host_type = ipv4;

    return true;
}

static void assign_port(struct request_parser *p) {
    switch (p->request->request_target.host_type) {
        case ipv4:
            p->request->request_target.host.ipv4.sin_port = p->request->request_target.port;
            break;
        case ipv6:
            p->request->request_target.host.ipv6.sin6_port = p->request->request_target.port;
            break;
        default:
            break;
    }
}

static enum request_state
r_target_port(const uint8_t c, struct request_parser *p) {
    if (p->i >= p->n)
        return request_error;

    if (END_OF_AUTHORITY(c)) {
        p->request->request_target.port = htons(p->request->request_target.port);
        assign_port(p);
        p->i = 0;
        p->n = MAX_ORIGIN_FORM;
        p->request->request_target.origin_form[p->i] = c;
        return request_target_ogform;
    }

    if (c == ' ') {
        p->request->request_target.port = htons(p->request->request_target.port);
        assign_port(p);
        p->i = 0;
        p->n = VERSION_LENGTH;
        return request_version;
    }

    if (!IS_DIGIT(c))
        return request_error;

    uint8_t num = c - '0';

    uint16_t port = p->request->request_target.port;

    if (p->i == 0)
        port = num;
    else
        port = port * 10 + num;

    p->i++;
    p->request->request_target.port = port;

    return request_target_port;
}

static enum request_state
r_target_ogform(const uint8_t c, struct request_parser *p) {
    if (p->i >= p->n)
        return request_error;

    if (c == ' ') {
        p->i = 0;
        p->n = VERSION_LENGTH;
        return request_version;
    }

    p->request->request_target.origin_form[p->i++] = c;

    return request_target_ogform;
}

static enum request_state
r_version(const uint8_t c, struct request_parser *p) {
    char *version = HTTP;

    if (p->i >= p->n || version[p->i] != c)
        return request_error;

    p->i++;

    if (c == '/') {
        p->i = 0;
        p->n = 2;  //FIXME: CREAR ENUM
        return request_version_major;
    }

    return request_version;;
}

static enum request_state
r_version_major(const uint8_t c, struct request_parser *p) {
    if (p->i >= p->n)
        return request_error_unsupported_version;

    if (c == '.') {
        p->i = 0;
        p->n = 2;  //FIXME: CREAR ENUM
        return request_version_minor;
    }

    if (!IS_DIGIT(c))
        return request_error;

    //FIXME:REVISAR ESTO
    uint8_t num = c - '0';

    uint16_t major = p->request->version_major;

    if (p->i == 0)
        major = num;
    else
        major = major * 10 + num;

    p->i++;
    p->request->version_major = major;

    return request_version_major;
}

static enum request_state
r_version_minor(const uint8_t c, struct request_parser *p) {
    if (p->i >= p->n)
        return request_error_unsupported_version;

    if (c == '\r') {
        p->i = 0;
        return request_end;
    }

    if (!IS_DIGIT(c))
        return request_error;

    uint8_t num = c - '0';

    uint16_t minor = p->request->version_minor;

    if (p->i == 0)
        minor = num;
    else
        minor = minor * 10 + num;

    p->i++;
    p->request->version_minor = minor;

    return request_version_minor;
}

static enum request_state
r_end(const uint8_t c, struct request_parser *p) {
    if (c != '\n')
        return request_error;

    return request_done;
}
