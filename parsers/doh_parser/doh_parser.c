#include "doh_parser.h"

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static enum doh_response_state
r_header_id(doh_response_parser_t *p, uint8_t c);

static enum doh_response_state
r_header_flags(doh_response_parser_t *p, uint8_t c);

static enum doh_response_state
r_header_qdcount(doh_response_parser_t *p, uint8_t c);

static enum doh_response_state
r_header_ancount(doh_response_parser_t *p, uint8_t c);

static enum doh_response_state
r_header_nscount(doh_response_parser_t *p, uint8_t c);

static enum doh_response_state
r_header_arcount(doh_response_parser_t *p, uint8_t c);

static enum doh_response_state
r_question_qname_label_length(doh_response_parser_t *p, uint8_t c);

static enum doh_response_state
r_question_qname_label(doh_response_parser_t *p, uint8_t c);

static enum doh_response_state
r_question_qtype(doh_response_parser_t *p, uint8_t c);

static enum doh_response_state
r_question_qclass(doh_response_parser_t *p, uint8_t c);

static enum doh_response_state
r_answer_name_pointer(doh_response_parser_t *p, uint8_t c);

static enum doh_response_state
r_answer_name_label_length(doh_response_parser_t *p, uint8_t c);

static enum doh_response_state
r_answer_name_label(doh_response_parser_t *p, uint8_t c);

static enum doh_response_state
r_answer_type(doh_response_parser_t *p, uint8_t c);

static enum doh_response_state
r_answer_class(doh_response_parser_t *p, uint8_t c);

static enum doh_response_state
r_answer_ttl(doh_response_parser_t *p, uint8_t c);

static enum doh_response_state
r_answer_rdlength(doh_response_parser_t *p, uint8_t c);

static enum doh_response_state
r_answer_ipv4_rdata(doh_response_parser_t *p, uint8_t c);

static enum doh_response_state
r_answer_ipv6_rdata(doh_response_parser_t *p, uint8_t c);

static enum doh_response_state
r_answer_cname_label_length_rdata(doh_response_parser_t *p, uint8_t c);

static enum doh_response_state
r_answer_cname_label_rdata(doh_response_parser_t *p, uint8_t c);

static enum doh_response_state
r_answer_cname_pointer_rdata(doh_response_parser_t *p, uint8_t c);

void doh_response_parser_init(doh_response_parser_t *p) {
    p->state = response_header_id;
    p->i = 0;
    p->n = HEADER_ID_LENGTH;
    memset(p->response, 0, sizeof(*(p->response)));
}

void doh_response_parser_destroy(doh_response_parser_t *p) {
    /*Chequeo que para hacer el free haya sido inicializado previamente*/
    if (p->response->answers != NULL) {
        free(p->response->answers);
        p->response->answers = NULL;
    }
}

enum doh_response_state
doh_response_parser_feed(doh_response_parser_t *p, uint8_t c) {
    enum doh_response_state next;

    switch (p->state) {
        case response_header_id:
            next = r_header_id(p, c);
            break;
        case response_header_flags:
            next = r_header_flags(p, c);
            break;
        case response_header_qdcount:
            next = r_header_qdcount(p, c);
            break;
        case response_header_ancount:
            next = r_header_ancount(p, c);
            break;
        case response_header_nscount:
            next = r_header_nscount(p, c);
            break;
        case response_header_arcount:
            next = r_header_arcount(p, c);
            break;
        case response_question_qname_label_length:
            next = r_question_qname_label_length(p, c);
            break;

        case response_question_qname_label:
            next = r_question_qname_label(p, c);
            break;
        case response_question_qtype:
            next = r_question_qtype(p, c);
            break;
        case response_question_qclass:
            next = r_question_qclass(p, c);
            break;

        case response_answer_name_label_length:
            next = r_answer_name_label_length(p, c);
            break;
        case response_answer_name_label:
            next = r_answer_name_label(p, c);
            break;
        case response_answer_name_pointer:
            next = r_answer_name_pointer(p, c);
            break;

        case response_answer_type:
            next = r_answer_type(p, c);
            break;
        case response_answer_class:
            next = r_answer_class(p, c);
            break;
        case response_answer_ttl:
            next = r_answer_ttl(p, c);
            break;
        case response_answer_rdlength:
            next = r_answer_rdlength(p, c);
            break;
        case response_answer_ipv4_rdata:
            next = r_answer_ipv4_rdata(p, c);
            break;
        case response_answer_ipv6_rdata:
            next = r_answer_ipv6_rdata(p, c);
            break;
        case response_answer_cname_label_rdata:
            next = r_answer_cname_label_rdata(p, c);
            break;
        case response_answer_cname_label_length_rdata:
            next = r_answer_cname_label_length_rdata(p, c);
            break;
        case response_answer_cname_pointer_rdata:
            next = r_answer_cname_pointer_rdata(p, c);
            break;

        case doh_response_done:
        case doh_no_answers:
        case response_mem_alloc_error:
        case doh_response_error:
            next = p->state;
            break;
        default:
            next = doh_response_error;
            break;
    }

    return p->state = next;
}

static enum doh_response_state
r_header_id(doh_response_parser_t *p, uint8_t c) {
    doh_response_state next;

    switch (p->i) {
        case 0:
            p->i++;
            p->response->header.id = ((uint16_t) c) << 8;
            next = response_header_id;
            break;

        case 1:
            p->response->header.id += (uint16_t) c;
            p->i = 0;
            p->n = HEADER_FLAGS_LENGTH;
            next = response_header_flags;
            break;

        default:
            next = doh_response_error;
            break;
    }
    return next;
}

static enum doh_response_state
r_header_flags(doh_response_parser_t *p, uint8_t c) {
    doh_response_state next;

    switch (p->i) {
        case 0:
            p->i++;
            p->response->header.flags.qr = c >> 7 & 0x01;
            p->response->header.flags.opcode = c >> 3 & 0x0F;
            p->response->header.flags.aa = c >> 2 & 0x01;
            p->response->header.flags.tc = c >> 1 & 0x01;
            p->response->header.flags.rd = c & 0x01;
            next = response_header_flags;
            break;
        case 1:
            p->response->header.flags.ra = c >> 7 & 0x01;
            p->response->header.flags.z = c >> 4 & 0x07;
            p->response->header.flags.rcode = c & 0x0F;
            p->i = 0;
            p->n = HEADER_QDCOUNT_LENGTH;
            next = response_header_qdcount;
            break;

        default:
            next = doh_response_error;
            break;
    }
    return next;
}

static enum doh_response_state
r_header_qdcount(doh_response_parser_t *p, uint8_t c) {
    doh_response_state next;

    switch (p->i) {
        case 0:
            p->i++;
            p->response->header.qdcount = ((uint16_t) c) << 8;
            next = response_header_qdcount;
            break;

        case 1:
            p->response->header.qdcount += (uint16_t) c;
            p->n = HEADER_ANCOUNT_LENGTH;
            p->i = 0;
            next = response_header_ancount;
            break;

        default:
            next = doh_response_error;
            break;
    }
    return next;
}

static enum doh_response_state
r_header_ancount(doh_response_parser_t *p, uint8_t c) {
    doh_response_state next;

    switch (p->i) {
        case 0:
            p->i++;
            p->response->header.ancount = ((uint16_t) c) << 8;
            next = response_header_ancount;
            break;

        case 1:
            p->response->header.ancount += (uint16_t) c;
            p->i = 0;
            p->n = HEADER_NSCOUNT_LENGTH;
            if (p->response->header.ancount == 0) {
                next = doh_no_answers;
                break;
            }
            p->response->answers = calloc(sizeof(struct answer), p->response->header.ancount);
            if (p->response->answers == NULL) {
                next = response_mem_alloc_error;
            } else {
                next = response_header_nscount;
            }
            break;

        default:
            next = doh_response_error;
            break;
    }
    return next;
}

static enum doh_response_state
r_header_nscount(doh_response_parser_t *p, uint8_t c) {
    doh_response_state next;

    switch (p->i) {
        case 0:
            p->i++;
            p->response->header.nscount = ((uint16_t) c) << 8;
            next = response_header_nscount;
            break;

        case 1:
            p->response->header.nscount += (uint16_t) c;
            p->i = 0;
            p->n = HEADER_ARCOUNT_LENGTH;
            next = response_header_arcount;
            break;

        default:
            next = doh_response_error;
            break;
    }
    return next;
}

static enum doh_response_state
r_header_arcount(doh_response_parser_t *p, uint8_t c) {
    doh_response_state next;

    switch (p->i) {
        case 0:
            p->i++;
            p->response->header.arcount = ((uint16_t) c) << 8;
            next = response_header_arcount;
            break;

        case 1:
            p->response->header.arcount += (uint16_t) c;
            next = response_question_qname_label_length;
            break;

        default:
            next = doh_response_error;
            break;
    }
    return next;
}

static enum doh_response_state
r_question_qname_label_length(doh_response_parser_t *p, uint8_t c) {
    if (c == 0x00) {
        p->i = 0;
        p->n = QUESTION_QTYPE_LENGTH;
        return response_question_qtype;
    }

    p->i = 0;
    p->n = c;

    return response_question_qname_label;
}

static enum doh_response_state
r_question_qname_label(doh_response_parser_t *p, uint8_t c) {
    if (p->i >= p->n)
        return doh_response_error;

    p->i++;
    p->response->question.qname[p->response->question.qname_size++] = c;

    if (p->i == p->n) {
        p->response->question.qname[p->response->question.qname_size++] = '.';
        return response_question_qname_label_length;
    }

    return response_question_qname_label;
}

static enum doh_response_state
r_question_qtype(doh_response_parser_t *p, uint8_t c) {
    doh_response_state next;

    switch (p->i) {
        case 0:
            p->i++;
            p->response->question.qtype = ((uint16_t) c) << 8;
            next = response_question_qtype;
            break;

        case 1:
            p->response->question.qtype += (uint16_t) c;
            p->n = QUESTION_QCLASS_LENGTH;
            p->i = 0;
            next = response_question_qclass;
            break;

        default:
            next = doh_response_error;
            break;
    }
    return next;
}

static enum doh_response_state
r_question_qclass(doh_response_parser_t *p, uint8_t c) {
    doh_response_state next;

    switch (p->i) {
        case 0:
            p->i++;
            p->response->question.qclass = ((uint16_t) c) << 8;
            next = response_question_qclass;
            break;

        case 1:
            p->response->question.qclass += (uint16_t) c;
            p->i = 0;
            next = response_answer_name_label_length;
            break;

        default:
            next = doh_response_error;
            break;
    }
    return next;
}

static enum doh_response_state
r_answer_name_label_length(doh_response_parser_t *p, uint8_t c) {
    if (c == 0x00) {
        p->i = 0;
        p->n = QUESTION_QTYPE_LENGTH;
        return response_answer_type;
    }
    // is a pointer
    if (c >> 6 == 0x03) {
        p->i = 0;
        p->n = ANSWER_NAME_PTR_LENGTH;
        p->i++;
        p->response->answers[p->response->answer_index].a_offset = (((uint16_t) c) & 0x3F) << 8;
        return response_answer_name_pointer;
    }

    p->i = 0;
    p->n = c;

    return response_answer_name_label;
}

static enum doh_response_state
r_answer_name_label(doh_response_parser_t *p, uint8_t c) {
    if (p->i >= p->n)
        return doh_response_error;

    p->i++;
    p->response->answers[p->response->answer_index].a_name.name[p->response->answers[p->response->answer_index].name_length++] = c;

    if (p->i == p->n) {
        p->response->answers[p->response->answer_index].a_name.name[p->response->answers[p->response->answer_index].name_length++] = '.';
        return response_answer_name_label_length;
    }

    return response_answer_name_label;
}

static enum doh_response_state
r_answer_name_pointer(doh_response_parser_t *p, uint8_t c) {
    doh_response_state next;

    switch (p->i) {
        case 1:
            p->response->answers[p->response->answer_index].a_offset += (uint16_t) c;
            p->response->answers[p->response->answer_index].a_name.ptr = p->response->question.qname;
            p->i = 0;
            p->n = ANSWER_TYPE_LENGTH;
            next = response_answer_type;
            break;

        default:
            next = doh_response_error;
            break;
    }
    return next;
}

static enum doh_response_state
r_answer_type(doh_response_parser_t *p, uint8_t c) {
    doh_response_state next;

    switch (p->i) {
        case 0:
            p->i++;
            p->response->answers[p->response->answer_index].a_type = ((uint16_t) c) << 8;
            next = response_answer_type;
            break;

        case 1:
            p->response->answers[p->response->answer_index].a_type += (uint16_t) c;
            p->i = 0;
            p->n = ANSWER_CLASS_LENGTH;
            next = response_answer_class;
            break;

        default:
            next = doh_response_error;
            break;
    }
    return next;
}

static enum doh_response_state
r_answer_class(doh_response_parser_t *p, uint8_t c) {
    doh_response_state next;

    switch (p->i) {
        case 0:
            p->i++;
            p->response->answers[p->response->answer_index].a_class = ((uint16_t) c) << 8;
            next = response_answer_class;
            break;

        case 1:
            p->response->answers[p->response->answer_index].a_class += (uint16_t) c;
            p->i = 0;
            p->n = ANSWER_TTL_LENGTH;
            next = response_answer_ttl;
            break;

        default:
            next = doh_response_error;
            break;
    }
    return next;
}

static enum doh_response_state
r_answer_ttl(doh_response_parser_t *p, uint8_t c) {
    doh_response_state next;

    switch (p->i) {
        case 0:
            p->response->answers[p->response->answer_index].a_ttl = ((uint32_t) c) << 24;
            p->i++;
            next = response_answer_ttl;
            break;

        case 1:
            p->response->answers[p->response->answer_index].a_ttl += ((uint32_t) c) << 16;
            p->i++;
            next = response_answer_ttl;
            break;

        case 2:
            p->response->answers[p->response->answer_index].a_ttl += ((uint32_t) c) << 8;
            p->i++;
            next = response_answer_ttl;
            break;

        case 3:
            p->response->answers[p->response->answer_index].a_ttl += (uint32_t) c;
            p->i = 0;
            p->n = ANSWER_RD_LENGTH;
            next = response_answer_rdlength;
            break;

        default:
            next = doh_response_error;
            break;
    }
    return next;
}

static enum doh_response_state
r_answer_rdlength(doh_response_parser_t *p, uint8_t c) {
    doh_response_state next;

    switch (p->i) {
        case 0:
            p->i++;
            p->response->answers[p->response->answer_index].a_rd_length = ((uint16_t) c) << 8;
            next = response_answer_rdlength;
            break;

        case 1:
            p->response->answers[p->response->answer_index].a_rd_length += (uint16_t) c;
            p->i = 0;
            switch (p->response->answers[p->response->answer_index].a_type) {
                case IPV4_TYPE:
                    p->n = MAX_IPV4;
                    next = response_answer_ipv4_rdata;
                    break;
                case IPV6_TYPE:
                    p->n = MAX_IPV6;
                    next = response_answer_ipv6_rdata;
                    break;
                case CNAME_TYPE:
                    p->n = MAX_CNAME_LENGTH;
                    next = response_answer_cname_label_length_rdata;
                    break;
                default:
                    next = doh_response_error;
            }
            break;

        default:
            next = doh_response_error;
            break;
    }
    return next;
}

static enum doh_response_state
r_answer_ipv4_rdata(doh_response_parser_t *p, uint8_t c) {
    if (p->i >= p->n)
        return doh_response_error;

    int total = sprintf((char *) p->response->answers[p->response->answer_index].ar_data +
                        p->response->answers[p->response->answer_index].a_rdata_length,
                        "%u", c);

    p->response->answers[p->response->answer_index].a_rdata_length += total;
    p->response->answers[p->response->answer_index].ar_data[p->response->answers[p->response->answer_index].a_rdata_length++] = '.';
    p->i++;
    if (p->i == p->n) {
        p->response->answers[p->response->answer_index].ar_data[--p->response->answers[p->response->answer_index].a_rdata_length] = 0;
        inet_pton(AF_INET, (char *) p->response->answers[p->response->answer_index].ar_data,
                  &p->response->answers[p->response->answer_index].aip.ipv4);

        /*No hay otra respuesta para analizar*/
        if (++p->response->answer_index == p->response->header.ancount)
            return doh_response_done;

        return response_answer_name_label_length;
    }

    return response_answer_ipv4_rdata;
}

static enum doh_response_state
r_answer_ipv6_rdata(doh_response_parser_t *p, uint8_t c) {
    if (p->i >= p->n)
        return doh_response_error;

    p->response->answers[p->response->answer_index].aip.ipv6.__in6_u.__u6_addr8[p->i++] = c;
    if (p->i == p->n) {
        p->response->answers[p->response->answer_index].a_rdata_length = INET6_ADDRSTRLEN - 1;
        inet_ntop(AF_INET6, &p->response->answers[p->response->answer_index].aip.ipv6,
                  (char *) p->response->answers[p->response->answer_index].ar_data,
                  p->response->answers[p->response->answer_index].a_rdata_length);

        /*No hay otra respuesta para analizar*/
        if (++p->response->answer_index == p->response->header.ancount)
            return doh_response_done;

        return response_answer_name_label_length;
    }

    return response_answer_ipv6_rdata;
}

/*No se guarda la data del CNAME*/
static enum doh_response_state
r_answer_cname_label_length_rdata(doh_response_parser_t *p, uint8_t c) {
    if (c == 0x00) {
        if (++p->response->answer_index == p->response->header.ancount)
            return doh_response_done;

        //Hay que leer otra respuesta
        return response_answer_name_label_length;
    }

    // is a pointer
    if (c >> 6 == 0x03) {
        p->i = 0;
        p->n = ANSWER_NAME_PTR_LENGTH;
        p->i++;
        return response_answer_cname_pointer_rdata;
    }

    p->i = 0;
    p->n = c;

    return response_answer_cname_label_rdata;
}

/*No se guarda la data del CNAME*/
static enum doh_response_state
r_answer_cname_label_rdata(doh_response_parser_t *p, uint8_t c) {
    if (p->i >= p->n)
        return doh_response_error;

    p->i++;

    if (p->i == p->n) {
        return response_answer_cname_label_length_rdata;
    }

    return response_answer_cname_label_rdata;
}

static enum doh_response_state
r_answer_cname_pointer_rdata(doh_response_parser_t *p, uint8_t c) {
    doh_response_state next;

    switch (p->i) {
        case 1:
            p->i = 0;
            if (++p->response->answer_index == p->response->header.ancount) {
                next = doh_response_done;
            } else {
                next = response_answer_name_label_length;
            }
            break;

        default:
            next = doh_response_error;
            break;
    }
    return next;
}
