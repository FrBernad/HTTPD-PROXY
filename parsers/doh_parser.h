#ifndef _DOH_PARSER_H_
#define _DOH_PARSER_H_

#define __packed __attribute__((packed))

#include <netinet/in.h>

/*
   labels         63 octets or less
   names           255 octets or less
   TTL             positive values of a signed 32 bit number.
   UDP messages    512 octets or less
   .www.example.com.
*/

enum values{
   /** max length for HTTP method */
   //HEADER
   HEADER_ID_LENGTH = 2,
   HEADER_FLAGS_LENGTH = 2,
   HEADER_QDCOUNT_LENGTH = 2,
   HEADER_ANCOUNT_LENGTH = 2,
   HEADER_NSCOUNT_LENGTH = 2,
   HEADER_ARCOUNT_LENGTH = 2,

   //QUESTION

   MAX_QUESTION_QNAME_LENGTH = 255,
   QUESTION_QTYPE_LENGTH = 2,
   QUESTION_QCLASS_LENGTH = 2,

   //ANSWER
   MAX_ANSWER_NAME_LENGTH = 255,
   ANSWER_TYPE_LENGTH = 2,
   ANSWER_CLASS_LENGTH = 2,
   ANSWER_TTL_LENGTH = 4,
   ANSWER_RD_LENGTH = 2,
   ANSWER_NAME_PTR_LENGTH = 2,

   MAX_CNAME_LENGTH = 255,

    IPV4 = 1,
    IPV6 = 28,
    CNAME = 5,
    MAX_IPV4 = 4,
    MAX_IPV6 = 16

};

struct answer{

    union {
        uint8_t *ptr;
        uint8_t name[MAX_ANSWER_NAME_LENGTH];
    } aname;
    uint8_t namelength;
    uint16_t aoffset;
    uint16_t atype;
    uint16_t aclass;
    uint32_t attl;
    uint16_t ardlength;
    uint16_t ardatalength;
    uint8_t ardata[INET6_ADDRSTRLEN];
    union {
        struct in6_addr ipv6;
        struct in_addr ipv4;
    } aip;

};

struct doh_response {

   struct {
       uint16_t id;
       struct __packed {
           unsigned qr : 1;
           unsigned opcode : 4;
           unsigned aa : 1;
           unsigned tc : 1;
           unsigned rd : 1;
           unsigned ra : 1;
           unsigned z : 3;
           unsigned rcode : 4;
       } flags;
       uint16_t qdcount;
       uint16_t ancount;
       uint16_t nscount;
       uint16_t arcount;
   } header;

   struct {
       uint8_t qnameSize;
       uint8_t qname[MAX_QUESTION_QNAME_LENGTH];
       uint16_t qtype;
       uint16_t qclass;
   } question;

  struct answer * answers;
  uint16_t answerIndex;

};

typedef enum doh_response_state {
    //Header
    response_header_id = 0,
    response_header_flags,
    response_header_qdcount,
    response_header_ancount,
    response_header_nscount,
    response_header_arcount,

    //Question
    response_question_qname_label_length,
    response_question_qname_label,
    response_question_qtype,
    response_question_qclass,

    //ANSWER
    response_answer_name_label_length,
    response_answer_name_label,
    response_answer_name_pointer,
    response_answer_type,
    response_answer_class,
    response_answer_ttl,
    response_answer_rdlength,
    response_answer_ipv4_rdata,
    response_answer_ipv6_rdata,
    response_answer_cname_label_rdata,
    response_answer_cname_label_length_rdata,
    response_answer_cname_pointer_rdata,

    //Done
    doh_response_done,

    //Error
    response_mem_alloc_error,
    doh_response_error,
} doh_response_state;

struct doh_response_parser {
   struct doh_response *response;
   doh_response_state state;
   int i;
   int n;
};

/** init parser */
void doh_response_parser_init(struct doh_response_parser *p);

/** returns true if done */
enum doh_response_state doh_response_parser_feed(struct doh_response_parser *p, const uint8_t c);

/*Libera el espacio allocado para guardar las respuestas de la consulta dns/ Debe llamarse luego de haber obtenido la respuesta*/
void doh_response_parser_destroy(struct doh_response_parser *p);

#endif
