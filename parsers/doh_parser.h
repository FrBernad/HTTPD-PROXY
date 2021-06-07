#ifndef _DOH_PARSER_H_
#define _DOH_PARSER_H_

#define __packed __attribute__((packed))

#include <netinet/in.h>

/*
  #Format

    +---------------------+
    |        Header       |
    +---------------------+
    |    question for the name server
    +---------------------+
    |        Answer       | RRs answering the question
    +---------------------+
    |      Authority      | RRs pointing toward an authority
    +---------------------+
    |      Additional     | RRs holding additional information
    +---------------------+

   #Header section format (12B)
                                    1  1  1  1  1  1
      0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                      ID                       |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |QR|   Opcode  |AA|TC|RD|RA|   Z    |   RCODE   |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                    QDCOUNT                    |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                    ANCOUNT                    |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                    NSCOUNT                    |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                    ARCOUNT                    |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+

    ID              A 16 bit identifier assigned by the program that
                    generates any kind of query.  This identifier is copied
                    the corresponding reply and can be used by the requester
                    to match up replies to outstanding queries.

    QR              A one bit field that specifies whether this message is a
                    query (0), or a response (1).

    OPCODE          A four bit field that specifies kind of query in this
                    message.  This value is set by the originator of a query
                    and copied into the response.  The values are:

                    0               a standard query (QUERY)

                    1               an inverse query (IQUERY)

                    2               a server status request (STATUS)

                    3-15            reserved for future use

    AA              Authoritative Answer - this bit is valid in responses,
                    and specifies that the responding name server is an
                    authority for the domain name in question section.

                    Note that the contents of the answer section may have
                    multiple owner names because of aliases.  The AA bit
                    corresponds to the name which matches the query name, or
                    the first owner name in the answer section.

    TC              TrunCation - specifies that this message was truncated
                    due to length greater than that permitted on the
                    transmission channel.

    RD              Recursion Desired - this bit may be set in a query and
                    is copied into the response.  If RD is set, it directs
                    the name server to pursue the query recursively.
                    Recursive query support is optional.

    RA              Recursion Available - this be is set or cleared in a
                    response, and denotes whether recursive query support is
                    available in the name server.

    Z               Reserved for future use.  Must be zero in all queries
                    and responses.

    RCODE           Response code - this 4 bit field is set as part of
                    responses.  The values have the following
                    interpretation:

                    0               No error condition

                    1               Format error - The name server was
                                    unable to interpret the query.

                    2               Server failure - The name server was
                                    unable to process this query due to a
                                    problem with the name server.

                    3               Name Error - Meaningful only for
                                    responses from an authoritative name
                                    server, this code signifies that the
                                    domain name referenced in the query does
                                    not exist.

                    4               Not Implemented - The name server does
                                    not support the requested kind of query.

                    5               Refused - The name server refuses to
                                    perform the specified operation for
                                    policy reasons.  For example, a name
                                    server may not wish to provide the
                                    information to the particular requester,
                                    or a name server may not wish to perform
                                    a particular operation (e.g., zone
                                    transfer) for particular data.

                    6-15            Reserved for future use.

    QDCOUNT         an unsigned 16 bit integer specifying the number of
                    entries in the question section.

    ANCOUNT         an unsigned 16 bit integer specifying the number of
                    resource records in the answer section.

    NSCOUNT         an unsigned 16 bit integer specifying the number of name
                    server resource records in the authority records
                    section.

    ARCOUNT         an unsigned 16 bit integer specifying the number of
                    resource records in the additional records section.

    #Resource record format
                                    1  1  1  1  1  1
      0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                                               |
    /                                               /
    /                      NAME                     /
    |                                               |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                      TYPE                     |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                     CLASS                     |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                      TTL                      |
    |                                               |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                   RDLENGTH                    |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--|
    /                     RDATA                     /
    /                                               /
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+

    NAME            a domain name to which this resource record pertains.

    TYPE            two octets containing one of the RR type codes.  This
                    field specifies the meaning of the data in the RDATA
                    field.

    CLASS           two octets which specify the class of the data in the
                    RDATA field.

    TTL             a 32 bit unsigned integer that specifies the time
                    interval (in seconds) that the resource record may be
                    cached before it should be discarded.  Zero values are
                    interpreted to mean that the RR can only be used for the
                    transaction in progress, and should not be cached.

    RDLENGTH        an unsigned 16 bit integer that specifies the length in
                    octets of the RDATA field.

    RDATA           a variable length string of octets that describes the
                    resource.  The format of this information varies
                    according to the TYPE and CLASS of the resource record.
                    For example, the if the TYPE is A and the CLASS is IN,
                    the RDATA field is a 4 octet ARPA Internet address.

    #Message compression

    In order to reduce the size of messages, the domain system utilizes a
    compression scheme which eliminates the repetition of domain names in a
    message.  In this scheme, an entire domain name or a list of labels at
    the end of a domain name is replaced with a pointer to a prior occurance
    of the same name.

    The pointer takes the form of a two octet sequence:

           0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5
        +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
        | 1  1|                OFFSET                   |
        +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+

    The compression scheme allows a domain name in a message to be
    represented as either:

    - a sequence of labels ending in a zero octet

    - a pointer

    - a sequence of labels ending with a pointer


    #Question section format
                                    1  1  1  1  1  1
      0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                                               |
    /                     QNAME                     /
    /                                               /
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                     QTYPE                     |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                     QCLASS                    |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+

    QNAME       a domain name represented as a sequence of labels, where
                each label consists of a length octet followed by that
                number of octets.  The domain name terminates with the
                zero length octet for the null label of the root.  Note
                that this field may be an odd number of octets; no
                padding is used.

    QTYPE       a two octet code which specifies the type of the query.
                The values for this field include all codes valid for a
                TYPE field, together with some more general codes which
                can match more than one type of RR.

    QCLASS      a two octet code that specifies the class of the query.
                For example, the QCLASS field is IN for the Internet.


    #TYPE values

    TYPE fields are used in resource records.  Note that these types are a
    subset of QTYPEs.

    TYPE            value and meaning

    AAAA            28 a single IPv6 address.

    A               1 a host address

    NS              2 an authoritative name server

    MD              3 a mail destination (Obsolete - use MX)

    MF              4 a mail forwarder (Obsolete - use MX)

    CNAME           5 the canonical name for an alias

    SOA             6 marks the start of a zone of authority

    MB              7 a mailbox domain name (EXPERIMENTAL)

    MG              8 a mail group member (EXPERIMENTAL)

    MR              9 a mail rename domain name (EXPERIMENTAL)

    NULL            10 a null RR (EXPERIMENTAL)

    WKS             11 a well known service description

    PTR             12 a domain name pointer

    HINFO           13 host information

    MINFO           14 mailbox or mail list information

    MX              15 mail exchange

    TXT             16 text strings



    #QTYPE values

    QTYPE fields appear in the question part of a query.  QTYPES are a
    superset of TYPEs, hence all TYPEs are valid QTYPEs.  In addition, the
    following QTYPEs are defined:

    AXFR            252 A request for a transfer of an entire zone

    MAILB           253 A request for mailbox-related records (MB, MG or MR)

    MAILA           254 A request for mail agent RRs (Obsolete - see MX)

    *               255 A request for all records

    #CLASS values

    CLASS fields appear in resource records.  The following CLASS mnemonics
    and values are defined:

    IN              1 the Internet

    CS              2 the CSNET class (Obsolete - used only for examples in
                    some obsolete RFCs)

    CH              3 the CHAOS class

    HS              4 Hesiod [Dyer 87]



   labels          63 octets or less
   names           255 octets or less
   TTL             positive values of a signed 32 bit number.
   UDP messages    512 octets or less
*/

enum values {
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

    IPV4_TYPE = 1,
    IPV6_TYPE = 28,
    CNAME_TYPE = 5,
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

